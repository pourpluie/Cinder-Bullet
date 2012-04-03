// Include header
#include "DynamicsWorld.h"

#include "RigidBody.h"
#include "SoftBody.h"

namespace bullet {

	// Import namespaces
	using namespace ci;
	using namespace ci::app;
	using namespace std;

	/////////////////////////////////////////////////////////////////////////////////////////////////

	DynamicsWorldRef DynamicsWorld::create()
	{
		return DynamicsWorldRef( new DynamicsWorld() );
	}

	// Constructor
	DynamicsWorld::DynamicsWorld()
	{

		// Setup physics environment
		mCollisionConfiguration = new btDefaultCollisionConfiguration();
		mDispatcher = new btCollisionDispatcher( mCollisionConfiguration );
		mBroadphase = new btDbvtBroadphase();
		mSolver = new btSequentialImpulseConstraintSolver();
	
		// Default dynamics
		mSoftBodyWorldInfo.air_density = 1.2f;
		mSoftBodyWorldInfo.m_broadphase = mBroadphase;
		mSoftBodyWorldInfo.m_dispatcher = mDispatcher;
		mSoftBodyWorldInfo.m_gravity.setValue( 0.0f, -10.0f, 0.0f );
		mSoftBodyWorldInfo.water_density = 0.0f;
		mSoftBodyWorldInfo.water_offset = 0.0f;
		mSoftBodyWorldInfo.water_normal = btVector3( 0.0f, 0.0f, 0.0f );
		mSoftBodyWorldInfo.m_sparsesdf.Initialize();

		// Build world
		mWorld = new btDiscreteDynamicsWorld( mDispatcher, mBroadphase, mSolver, mCollisionConfiguration );
		mWorld->setGravity( btVector3( 0.0f, -10.0f, 0.0f ) );
		mWorld->getDispatchInfo().m_enableSPU = true;

		// Set tracking properties
		mElapsedSeconds = getElapsedSeconds();
		mNumObjects = 0;

	}

	// Destructor
	DynamicsWorld::~DynamicsWorld()
	{
		if ( mBroadphase != 0 ) {
			delete mBroadphase;
		}
		if ( mCollisionConfiguration != 0 ) {
			delete mCollisionConfiguration;
		}
		if ( mDispatcher != 0 ) {
			delete mDispatcher;
		}
		if ( mWorld != 0 ) {
			delete mWorld;
		}
		if ( mSolver != 0 ) {
			delete mSolver;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////

	Iter DynamicsWorld::begin()
	{
		return mObjects.begin();
	}
	
	Iter DynamicsWorld::end()
	{
		return mObjects.end();
	}
	
	Iter DynamicsWorld::erase( Iter pos )
	{
		if ( pos->isRigidBody() ) {
			mWorld->removeRigidBody( toBulletRigidBody( pos ) );
		} else if ( pos->isSoftBody() ) {
			( (btSoftRigidDynamicsWorld*)mWorld )->removeSoftBody( toBulletSoftBody( pos ) );
		}
		return mObjects.erase( pos );
	}

	void DynamicsWorld::pushBack( CollisionObject *object )
	{
		mObjects.push_back( object );
		if ( object->isRigidBody() ) {
			mWorld->addRigidBody( toBulletRigidBody( object ) );
		} else if ( object->isSoftBody() ) {
			( (btSoftRigidDynamicsWorld*)mWorld )->addSoftBody( toBulletSoftBody( object ) );
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////

	btBroadphaseInterface* DynamicsWorld::getBroadphase() 
	{ 
		return mBroadphase; 
	}
	btDefaultCollisionConfiguration* DynamicsWorld::getCollisionConfiguration() 
	{ 
		return mCollisionConfiguration; 
	}
	btCollisionDispatcher* DynamicsWorld::getDispatcher() 
	{ 
		return mDispatcher; 
	}
	btSoftBodyWorldInfo& DynamicsWorld::getInfo() 
	{ 
		return mSoftBodyWorldInfo; 
	}
	uint32_t DynamicsWorld::getNumObjects()
	{
		return mNumObjects;
	}
	CollisionObjectList& DynamicsWorld::getObjects()
	{
		return mObjects;
	}
	btConstraintSolver* DynamicsWorld::getSolver() 
	{ 
		return mSolver; 
	}
	btDiscreteDynamicsWorld* DynamicsWorld::getWorld() 
	{ 
		return mWorld; 
	}

	void DynamicsWorld::setInfo( const btSoftBodyWorldInfo &info )
	{
		mSoftBodyWorldInfo = info;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////

	// Runs update logic
	void DynamicsWorld::update()
	{

		// Check if object count has changed
		uint32_t numObjects = mWorld->getCollisionObjectArray().size();
		if ( mNumObjects != numObjects ) {

			// Nothing to do, bail
			if ( numObjects <= 0 ) {
				return;
			}

			// Activate all bodies
			for ( uint32_t i = 0; i < numObjects; i++ ) {
				btCollisionObject * collisionObject = mWorld->getCollisionObjectArray()[ i ];
				btRigidBody* rigidBody = btRigidBody::upcast( collisionObject );
				if ( rigidBody ) {
					rigidBody->activate( true );
				} else {
					btSoftBody* softBody = btSoftBody::upcast( collisionObject );
					if ( softBody ) {
						softBody->activate( true );
					}
				}
			}

		}

		// Update object count
		mNumObjects = numObjects;

		// Update dynamics world
		mWorld->stepSimulation( 1.0f, 10, 1.0f / math<float>::max( 1.0f, getFrameRate() ) );

	}

	/////////////////////////////////////////////////////////////////////////////////////////////////

	btRigidBody* DynamicsWorld::toBulletRigidBody( CollisionObject *object )
	{
		return (btRigidBody*)( object->getBulletBody() );
	}
	btRigidBody* DynamicsWorld::toBulletRigidBody( Iter pos )
	{
		return (btRigidBody*)( pos->getBulletBody() );
	}
	btSoftBody* DynamicsWorld::toBulletSoftBody( CollisionObject *object )
	{
		return (btSoftBody*)( object->getBulletBody() );
	}
	btSoftBody* DynamicsWorld::toBulletSoftBody( Iter pos )
	{
		return (btSoftBody*)( pos->getBulletBody() );
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////

	DynamicsWorldRef createWorld()
	{
		return DynamicsWorld::create();
	}

	CollisionObjectRef createRigidBox( const DynamicsWorldRef &world, const ci::Vec3f &dimensions, float mass, const ci::Vec3f &position, const ci::Quatf &rotation )
	{
		world->pushBack( new RigidBox( dimensions, mass, position, rotation ) );
		return &world->mObjects[ world->mObjects.size() - 1 ];
	}
	CollisionObjectRef createRigidCylinder( const DynamicsWorldRef &world, float topRadius, float bottomRadius, float height, int32_t segments, float mass, 
		const ci::Vec3f &position, const ci::Quatf &rotation )
	{
		world->pushBack( new RigidCylinder( topRadius, bottomRadius, height, segments, mass, position, rotation ) );
		return &world->mObjects[ world->mObjects.size() - 1 ];
	}
	CollisionObjectRef createRigidHull( const DynamicsWorldRef &world, const ci::TriMesh &mesh, const ci::Vec3f &scale, float mass, const ci::Vec3f &position, const ci::Quatf &rotation )
	{
		world->pushBack( new RigidHull( mesh, scale, mass, position, rotation ) );
		return &world->mObjects[ world->mObjects.size() - 1 ];
	}
	CollisionObjectRef createRigidMesh( const DynamicsWorldRef &world, const ci::TriMesh &mesh, const ci::Vec3f &scale, float margin, float mass, 
		const ci::Vec3f &position, const ci::Quatf &rotation )
	{
		world->pushBack( new RigidMesh( mesh, scale, margin, mass, position, rotation ) );
		return &world->mObjects[ world->mObjects.size() - 1 ];
	}
	CollisionObjectRef createRigidSphere( const DynamicsWorldRef &world, float radius, int32_t segments, float mass, const ci::Vec3f &position, const ci::Quatf &rotation )
	{
		world->pushBack( new RigidSphere( radius, segments, mass, position, rotation ) );
		return &world->mObjects[ world->mObjects.size() - 1 ];
	}
	CollisionObjectRef createRigidTerrain( const DynamicsWorldRef &world, const ci::Surface32f &heightField, int32_t stickWidth, int32_t stickLength, float minHeight, 
		float maxHeight, int32_t upAxis, const ci::Vec3f &scale, float mass, const ci::Vec3f &position, const ci::Quatf &rotation )
	{
		world->pushBack( new RigidTerrain( heightField, stickWidth, stickLength, minHeight, maxHeight, upAxis, scale, mass, position, rotation ) );
		return &world->mObjects[ world->mObjects.size() - 1 ];
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////

	btRigidBody* toBulletRigidBody( const CollisionObjectRef &object )
	{
		return (btRigidBody*)( object->getBulletBody() );
	}
	btSoftBody* toBulletSoftBody( const CollisionObjectRef &object )
	{
		return (btSoftBody*)( object->getBulletBody() );
	}

}
