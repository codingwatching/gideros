TARGET=reactphysics3d

INCS += ../../../Sdk/include
INCS += ../../../Sdk/include/gideros
INCS += ../../../2dsg
INCS += ../../../2dsg/gfxbackends
INCS += ../../../libgideros
INCS += ../../../libgid/include
INCS += ../../../luabinding
INCS += ../../../$(LUA_INCLUDE)

INCS += ../reactphysics3d/include

FSRCS += Common/reactbinder.cpp \
 	 	../../../luabinding/binder.cpp \
 	 	../reactphysics3d/src/body/Body.cpp \
		../reactphysics3d/src/body/RigidBody.cpp \
		../reactphysics3d/src/collision/broadphase/DynamicAABBTree.cpp \
		../reactphysics3d/src/collision/Collider.cpp \
		../reactphysics3d/src/collision/CollisionCallback.cpp \
		../reactphysics3d/src/collision/ContactManifold.cpp \
		../reactphysics3d/src/collision/ConvexMesh.cpp \
		../reactphysics3d/src/collision/HalfEdgeStructure.cpp \
		../reactphysics3d/src/collision/HeightField.cpp \
		../reactphysics3d/src/collision/narrowphase/CapsuleVsCapsuleAlgorithm.cpp \
		../reactphysics3d/src/collision/narrowphase/CapsuleVsConvexPolyhedronAlgorithm.cpp \
		../reactphysics3d/src/collision/narrowphase/CollisionDispatch.cpp \
		../reactphysics3d/src/collision/narrowphase/ConvexPolyhedronVsConvexPolyhedronAlgorithm.cpp \
		../reactphysics3d/src/collision/narrowphase/GJK/GJKAlgorithm.cpp \
		../reactphysics3d/src/collision/narrowphase/GJK/VoronoiSimplex.cpp \
		../reactphysics3d/src/collision/narrowphase/NarrowPhaseInfoBatch.cpp \
		../reactphysics3d/src/collision/narrowphase/NarrowPhaseInput.cpp \
		../reactphysics3d/src/collision/narrowphase/SAT/SATAlgorithm.cpp \
		../reactphysics3d/src/collision/narrowphase/SphereVsCapsuleAlgorithm.cpp \
		../reactphysics3d/src/collision/narrowphase/SphereVsConvexPolyhedronAlgorithm.cpp \
		../reactphysics3d/src/collision/narrowphase/SphereVsSphereAlgorithm.cpp \
		../reactphysics3d/src/collision/OverlapCallback.cpp \
		../reactphysics3d/src/collision/PolygonVertexArray.cpp \
		../reactphysics3d/src/collision/RaycastInfo.cpp \
		../reactphysics3d/src/collision/shapes/AABB.cpp \
		../reactphysics3d/src/collision/shapes/BoxShape.cpp \
		../reactphysics3d/src/collision/shapes/CapsuleShape.cpp \
		../reactphysics3d/src/collision/shapes/CollisionShape.cpp \
		../reactphysics3d/src/collision/shapes/ConcaveMeshShape.cpp \
		../reactphysics3d/src/collision/shapes/ConcaveShape.cpp \
		../reactphysics3d/src/collision/shapes/ConvexMeshShape.cpp \
		../reactphysics3d/src/collision/shapes/ConvexPolyhedronShape.cpp \
		../reactphysics3d/src/collision/shapes/ConvexShape.cpp \
		../reactphysics3d/src/collision/shapes/HeightFieldShape.cpp \
		../reactphysics3d/src/collision/shapes/SphereShape.cpp \
		../reactphysics3d/src/collision/shapes/TriangleShape.cpp \
		../reactphysics3d/src/collision/TriangleMesh.cpp \
		../reactphysics3d/src/collision/TriangleVertexArray.cpp \
		../reactphysics3d/src/collision/VertexArray.cpp \
		../reactphysics3d/src/components/BallAndSocketJointComponents.cpp \
		../reactphysics3d/src/components/BodyComponents.cpp \
		../reactphysics3d/src/components/ColliderComponents.cpp \
		../reactphysics3d/src/components/Components.cpp \
		../reactphysics3d/src/components/FixedJointComponents.cpp \
		../reactphysics3d/src/components/HingeJointComponents.cpp \
		../reactphysics3d/src/components/JointComponents.cpp \
		../reactphysics3d/src/components/RigidBodyComponents.cpp \
		../reactphysics3d/src/components/SliderJointComponents.cpp \
		../reactphysics3d/src/components/TransformComponents.cpp \
		../reactphysics3d/src/constraint/BallAndSocketJoint.cpp \
		../reactphysics3d/src/constraint/ContactPoint.cpp \
		../reactphysics3d/src/constraint/FixedJoint.cpp \
		../reactphysics3d/src/constraint/HingeJoint.cpp \
		../reactphysics3d/src/constraint/Joint.cpp \
		../reactphysics3d/src/constraint/SliderJoint.cpp \
		../reactphysics3d/src/engine/Entity.cpp \
		../reactphysics3d/src/engine/EntityManager.cpp \
		../reactphysics3d/src/engine/Island.cpp \
		../reactphysics3d/src/engine/Material.cpp \
		../reactphysics3d/src/engine/OverlappingPairs.cpp \
		../reactphysics3d/src/engine/PhysicsCommon.cpp \
		../reactphysics3d/src/engine/PhysicsWorld.cpp \
		../reactphysics3d/src/mathematics/Matrix2x2.cpp \
		../reactphysics3d/src/mathematics/Matrix3x3.cpp \
		../reactphysics3d/src/mathematics/Quaternion.cpp \
		../reactphysics3d/src/mathematics/Transform.cpp \
		../reactphysics3d/src/mathematics/Vector2.cpp \
		../reactphysics3d/src/mathematics/Vector3.cpp \
		../reactphysics3d/src/memory/HeapAllocator.cpp \
		../reactphysics3d/src/memory/MemoryAllocator.cpp \
		../reactphysics3d/src/memory/MemoryManager.cpp \
		../reactphysics3d/src/memory/PoolAllocator.cpp \
		../reactphysics3d/src/memory/SingleFrameAllocator.cpp \
		../reactphysics3d/src/systems/BroadPhaseSystem.cpp \
		../reactphysics3d/src/systems/CollisionDetectionSystem.cpp \
		../reactphysics3d/src/systems/ConstraintSolverSystem.cpp \
		../reactphysics3d/src/systems/ContactSolverSystem.cpp \
		../reactphysics3d/src/systems/DynamicsSystem.cpp \
		../reactphysics3d/src/systems/SolveBallAndSocketJointSystem.cpp \
		../reactphysics3d/src/systems/SolveFixedJointSystem.cpp \
		../reactphysics3d/src/systems/SolveHingeJointSystem.cpp \
		../reactphysics3d/src/systems/SolveSliderJointSystem.cpp \
		../reactphysics3d/src/utils/quickhull/QHHalfEdgeStructure.cpp \
		../reactphysics3d/src/utils/quickhull/QuickHull.cpp \
		../reactphysics3d/src/utils/DebugRenderer.cpp \
		../reactphysics3d/src/utils/DefaultLogger.cpp \
  		../reactphysics3d/src/utils/Profiler.cpp 
SRCS+=$(basename $(FSRCS)) 
CFLGS+=-frtti

include ../../../../emscripten/Plugin.mk
