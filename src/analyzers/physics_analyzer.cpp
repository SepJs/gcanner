#include <game_req_analyzer/analyzers/physics_analyzer.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <algorithm>
#include <fstream>
#include <regex>

namespace game_req {

PhysicsAnalyzer::PhysicsAnalyzer(const AnalyzerConfig& config) : config_(config) {}

Result<std::vector<PhysicsInfo>> PhysicsAnalyzer::analyze(const std::vector<FileInfo>& physics_files) {
    std::vector<PhysicsInfo> results;
    results.reserve(physics_files.size());
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_scenes = 0;
        stats_.total_bodies = 0;
        stats_.total_shapes = 0;
        stats_.total_constraints = 0;
        stats_.total_disk_size = 0;
        stats_.total_estimated_ram = 0;
        stats_.format_counts.clear();
        stats_.engine_counts.clear();
        stats_.shape_type_counts.clear();
        stats_.max_bodies_per_scene = 0;
    }
    
    for (const auto& physics : physics_files) {
        auto result = analyze_single(physics);
        if (result) {
            results.push_back(*result);
            
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_scenes++;
            stats_.total_bodies += result->body_count;
            stats_.total_shapes += result->shape_count;
            stats_.total_constraints += result->constraint_count;
            stats_.total_disk_size += result->disk_size;
            stats_.total_estimated_ram += result->estimated_ram;
            stats_.format_counts[result->format]++;
            if (!result->engine_hint.empty()) stats_.engine_counts[result->engine_hint]++;
            for (const auto& shape : result->shape_types) {
                stats_.shape_type_counts[shape]++;
            }
            if (result->body_count > stats_.max_bodies_per_scene) {
                stats_.max_bodies_per_scene = result->body_count;
            }
        } else {
            LOG_WARN("Failed to analyze physics file {}: {}", physics.path.string(), result.error().message);
        }
    }
    
    return results;
}

Result<PhysicsInfo> PhysicsAnalyzer::analyze_single(const FileInfo& physics) {
    PhysicsInfo info;
    info.disk_size = physics.size;
    
    try {
        switch (physics.type) {
            case FileType::PHYSX:
            case FileType::PHYSX3:
                return analyze_physx(physics.path);
            case FileType::HAVOK:
                return analyze_havok(physics.path);
            case FileType::BULLET:
                return analyze_bullet(physics.path);
            case FileType::ODE:
                return analyze_ode(physics.path);
            case FileType::NEWTON:
                return analyze_newton(physics.path);
            case FileType::PXD:
                return analyze_pxd(physics.path);
            case FileType::PXB:
                return analyze_pxb(physics.path);
            default:
                return analyze_generic(physics.path, physics.type);
        }
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Physics analysis failed: {}", e.what())));
    }
}

String PhysicsAnalyzer::read_file_content(const Path& path) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return "";
    
    std::streamsize size = file.tellg();
    if (size > 1024 * 1024) return ""; // Skip large files for text analysis
    
    file.seekg(0, std::ios::beg);
    String content;
    content.resize(size);
    if (size > 0) file.read(content.data(), size);
    return content;
}

Result<PhysicsInfo> PhysicsAnalyzer::analyze_physx(const Path& path) {
    PhysicsInfo info;
    info.format = "PhysX";
    info.disk_size = fs::file_size(path);
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_physics_engine(info, content);
        
        // PhysX binary formats - look for specific patterns
        std::regex body_regex(R"(PxRigidBody|PxRigidDynamic|PxRigidStatic)");
        info.body_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), body_regex),
            std::sregex_iterator()
        );
        
        std::regex shape_regex(R"(PxShape|PxGeometry|PxBoxGeometry|PxSphereGeometry|PxCapsuleGeometry|PxConvexMeshGeometry|PxTriangleMeshGeometry|PxHeightFieldGeometry)");
        info.shape_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), shape_regex),
            std::sregex_iterator()
        );
        
        std::regex constraint_regex(R"(PxConstraint|PxJoint|PxDistanceJoint|PxRevoluteJoint|PxPrismaticJoint|PxSphericalJoint|PxFixedJoint|PxD6Joint)");
        info.constraint_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), constraint_regex),
            std::sregex_iterator()
        );
        
        // Detect shape types
        if (content.find("PxBoxGeometry") != String::npos) info.shape_types.push_back("Box");
        if (content.find("PxSphereGeometry") != String::npos) info.shape_types.push_back("Sphere");
        if (content.find("PxCapsuleGeometry") != String::npos) info.shape_types.push_back("Capsule");
        if (content.find("PxConvexMeshGeometry") != String::npos) info.shape_types.push_back("Convex Mesh");
        if (content.find("PxTriangleMeshGeometry") != String::npos) info.shape_types.push_back("Triangle Mesh");
        if (content.find("PxHeightFieldGeometry") != String::npos) info.shape_types.push_back("HeightField");
        
        // Features
        info.has_continuous_collision = content.find("PxCCD") != String::npos || content.find("continuous") != String::npos;
        info.has_vehicle_support = content.find("PxVehicle") != String::npos;
        info.has_character_controller = content.find("PxController") != String::npos || content.find("PxCharacterController") != String::npos;
        info.has_soft_body = content.find("PxSoftBody") != String::npos || content.find("PxCloth") != String::npos;
        info.has_particles = content.find("PxParticle") != String::npos;
    }
    
    if (info.body_count == 0 && info.shape_count == 0) {
        // Estimate from file size
        info.body_count = std::max(1UL, info.disk_size / (32 * 1024)); // ~32KB per body
        info.shape_count = info.body_count * 2;
        info.constraint_count = info.body_count / 4;
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<PhysicsInfo> PhysicsAnalyzer::analyze_havok(const Path& path) {
    PhysicsInfo info;
    info.format = "Havok";
    info.disk_size = fs::file_size(path);
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_physics_engine(info, content);
        
        // Havok patterns
        std::regex body_regex(R"(hkpRigidBody|hkpWorld|hkpSimulationIsland)");
        info.body_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), body_regex),
            std::sregex_iterator()
        );
        
        std::regex shape_regex(R"(hkpShape|hkpBoxShape|hkpSphereShape|hkpCapsuleShape|hkpConvexShape|hkpMeshShape|hkpHeightFieldShape)");
        info.shape_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), shape_regex),
            std::sregex_iterator()
        );
        
        std::regex constraint_regex(R"(hkpConstraint|hkpLimitedHingeConstraint|hkpBallSocketConstraint|hkpPrismaticConstraint|hkpRagdollConstraint)");
        info.constraint_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), constraint_regex),
            std::sregex_iterator()
        );
        
        // Shape types
        if (content.find("hkpBoxShape") != String::npos) info.shape_types.push_back("Box");
        if (content.find("hkpSphereShape") != String::npos) info.shape_types.push_back("Sphere");
        if (content.find("hkpCapsuleShape") != String::npos) info.shape_types.push_back("Capsule");
        if (content.find("hkpConvexShape") != String::npos) info.shape_types.push_back("Convex");
        if (content.find("hkpMeshShape") != String::npos) info.shape_types.push_back("Mesh");
        if (content.find("hkpHeightFieldShape") != String::npos) info.shape_types.push_back("HeightField");
        
        // Features
        info.has_continuous_collision = content.find("hkpContinuous") != String::npos;
        info.has_vehicle_support = content.find("hkpVehicle") != String::npos;
        info.has_character_controller = content.find("hkpCharacter") != String::npos;
        info.has_cloth = content.find("hkpCloth") != String::npos;
        info.has_particles = content.find("hkpParticle") != String::npos;
    }
    
    if (info.body_count == 0 && info.shape_count == 0) {
        info.body_count = std::max(1UL, info.disk_size / (48 * 1024));
        info.shape_count = info.body_count * 2;
        info.constraint_count = info.body_count / 4;
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<PhysicsInfo> PhysicsAnalyzer::analyze_bullet(const Path& path) {
    PhysicsInfo info;
    info.format = "Bullet";
    info.disk_size = fs::file_size(path);
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_physics_engine(info, content);
        
        // Bullet patterns
        std::regex body_regex(R"(btRigidBody|btCollisionObject)");
        info.body_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), body_regex),
            std::sregex_iterator()
        );
        
        std::regex shape_regex(R"(btCollisionShape|btBoxShape|btSphereShape|btCapsuleShape|btCylinderShape|btConeShape|btConvexHullShape|btTriangleMeshShape|btHeightfieldTerrainShape|btConvexTriangleMeshShape)");
        info.shape_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), shape_regex),
            std::sregex_iterator()
        );
        
        std::regex constraint_regex(R"(btTypedConstraint|btHingeConstraint|btSliderConstraint|btConeTwistConstraint|btGeneric6DofConstraint|btPoint2PointConstraint|btFixedConstraint)");
        info.constraint_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), constraint_regex),
            std::sregex_iterator()
        );
        
        // Shape types
        if (content.find("btBoxShape") != String::npos) info.shape_types.push_back("Box");
        if (content.find("btSphereShape") != String::npos) info.shape_types.push_back("Sphere");
        if (content.find("btCapsuleShape") != String::npos) info.shape_types.push_back("Capsule");
        if (content.find("btCylinderShape") != String::npos) info.shape_types.push_back("Cylinder");
        if (content.find("btConeShape") != String::npos) info.shape_types.push_back("Cone");
        if (content.find("btConvexHullShape") != String::npos) info.shape_types.push_back("Convex Hull");
        if (content.find("btTriangleMeshShape") != String::npos) info.shape_types.push_back("Triangle Mesh");
        if (content.find("btHeightfieldTerrainShape") != String::npos) info.shape_types.push_back("HeightField");
        if (content.find("btConvexTriangleMeshShape") != String::npos) info.shape_types.push_back("Convex Triangle Mesh");
        
        // Features
        info.has_continuous_collision = content.find("btGjkEpaPenetrationDepthSolver") != String::npos || 
                                        content.find("continuous") != String::npos;
        info.has_vehicle_support = content.find("btRaycastVehicle") != String::npos;
        info.has_character_controller = content.find("btKinematicCharacterController") != String::npos;
        info.has_soft_body = content.find("btSoftBody") != String::npos;
        info.has_fluid = content.find("btFluid") != String::npos;
    }
    
    if (info.body_count == 0 && info.shape_count == 0) {
        info.body_count = std::max(1UL, info.disk_size / (24 * 1024));
        info.shape_count = info.body_count * 2;
        info.constraint_count = info.body_count / 4;
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<PhysicsInfo> PhysicsAnalyzer::analyze_ode(const Path& path) {
    PhysicsInfo info;
    info.format = "ODE";
    info.disk_size = fs::file_size(path);
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_physics_engine(info, content);
        
        // ODE patterns
        std::regex body_regex(R"(dBodyCreate|dBodyID)");
        info.body_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), body_regex),
            std::sregex_iterator()
        );
        
        std::regex shape_regex(R"(dCreateBox|dCreateSphere|dCreateCapsule|dCreateCylinder|dCreateRay|dCreatePlane|dCreateConvex|dCreateTriMesh)");
        info.shape_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), shape_regex),
            std::sregex_iterator()
        );
        
        std::regex constraint_regex(R"(dJointCreate|dJointGroupCreate|dJointAttach)");
        info.constraint_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), constraint_regex),
            std::sregex_iterator()
        );
        
        // Shape types
        if (content.find("dCreateBox") != String::npos) info.shape_types.push_back("Box");
        if (content.find("dCreateSphere") != String::npos) info.shape_types.push_back("Sphere");
        if (content.find("dCreateCapsule") != String::npos) info.shape_types.push_back("Capsule");
        if (content.find("dCreateCylinder") != String::npos) info.shape_types.push_back("Cylinder");
        if (content.find("dCreateRay") != String::npos) info.shape_types.push_back("Ray");
        if (content.find("dCreatePlane") != String::npos) info.shape_types.push_back("Plane");
        if (content.find("dCreateConvex") != String::npos) info.shape_types.push_back("Convex");
        if (content.find("dCreateTriMesh") != String::npos) info.shape_types.push_back("Triangle Mesh");
        
        // Features
        info.has_continuous_collision = false; // ODE doesn't have CCD by default
        info.has_vehicle_support = content.find("dCreateAMotor") != String::npos || content.find("vehicle") != String::npos;
    }
    
    if (info.body_count == 0 && info.shape_count == 0) {
        info.body_count = std::max(1UL, info.disk_size / (16 * 1024));
        info.shape_count = info.body_count * 2;
        info.constraint_count = info.body_count / 4;
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<PhysicsInfo> PhysicsAnalyzer::analyze_newton(const Path& path) {
    PhysicsInfo info;
    info.format = "Newton";
    info.disk_size = fs::file_size(path);
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_physics_engine(info, content);
        
        // Newton patterns
        std::regex body_regex(R"(NewtonBodyCreate|NewtonDynamicBodyCreate|NewtonKinematicBodyCreate)");
        info.body_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), body_regex),
            std::sregex_iterator()
        );
        
        std::regex shape_regex(R"(NewtonCreateBox|NewtonCreateSphere|NewtonCreateCapsule|NewtonCreateCylinder|NewtonCreateConvexHull|NewtonCreateTreeCollision|NewtonCreateHeightField)");
        info.shape_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), shape_regex),
            std::sregex_iterator()
        );
        
        std::regex constraint_regex(R"(NewtonConstraintCreate|NewtonBallAndSocket|NewtonHinge|NewtonSlider|NewtonUniversal|NewtonCorkscrew|NewtonUserJoint)");
        info.constraint_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), constraint_regex),
            std::sregex_iterator()
        );
        
        // Shape types
        if (content.find("NewtonCreateBox") != String::npos) info.shape_types.push_back("Box");
        if (content.find("NewtonCreateSphere") != String::npos) info.shape_types.push_back("Sphere");
        if (content.find("NewtonCreateCapsule") != String::npos) info.shape_types.push_back("Capsule");
        if (content.find("NewtonCreateCylinder") != String::npos) info.shape_types.push_back("Cylinder");
        if (content.find("NewtonCreateConvexHull") != String::npos) info.shape_types.push_back("Convex Hull");
        if (content.find("NewtonCreateTreeCollision") != String::npos) info.shape_types.push_back("Triangle Mesh");
        if (content.find("NewtonCreateHeightField") != String::npos) info.shape_types.push_back("HeightField");
        
        // Features
        info.has_continuous_collision = content.find("NewtonSetContinuousCollisionMode") != String::npos;
        info.has_vehicle_support = content.find("NewtonVehicleCreate") != String::npos;
        info.has_character_controller = content.find("NewtonCharacterController") != String::npos;
        info.has_soft_body = content.find("NewtonSoftBody") != String::npos;
    }
    
    if (info.body_count == 0 && info.shape_count == 0) {
        info.body_count = std::max(1UL, info.disk_size / (20 * 1024));
        info.shape_count = info.body_count * 2;
        info.constraint_count = info.body_count / 4;
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<PhysicsInfo> PhysicsAnalyzer::analyze_physx3(const Path& path) {
    // PhysX 3.x format - similar to PhysX but newer
    return analyze_physx(path);
}

Result<PhysicsInfo> PhysicsAnalyzer::analyze_pxd(const Path& path) {
    // PhysX binary format (.pxd)
    PhysicsInfo info;
    info.format = "PhysX Binary (PXD)";
    info.disk_size = fs::file_size(path);
    
    // Estimate from file size
    info.body_count = std::max(1UL, info.disk_size / (32 * 1024));
    info.shape_count = info.body_count * 2;
    info.constraint_count = info.body_count / 4;
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<PhysicsInfo> PhysicsAnalyzer::analyze_pxb(const Path& path) {
    // PhysX binary format (.pxb)
    PhysicsInfo info;
    info.format = "PhysX Binary (PXB)";
    info.disk_size = fs::file_size(path);
    
    info.body_count = std::max(1UL, info.disk_size / (32 * 1024));
    info.shape_count = info.body_count * 2;
    info.constraint_count = info.body_count / 4;
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<PhysicsInfo> PhysicsAnalyzer::analyze_generic(const Path& path, FileType type) {
    PhysicsInfo info;
    info.disk_size = fs::file_size(path);
    info.format = "Unknown";
    
    info.body_count = std::max(1UL, info.disk_size / (24 * 1024));
    info.shape_count = info.body_count * 2;
    info.constraint_count = info.body_count / 4;
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

u64 PhysicsAnalyzer::estimate_ram_usage(const PhysicsInfo& physics) const {
    // Physics engine RAM usage:
    // - Rigid bodies: ~1-2 KB each (position, velocity, inertia, force accumulators)
    // - Shapes: ~0.5-2 KB each depending on complexity
    // - Constraints: ~1-4 KB each
    // - Broadphase structures (BVH, SAP): ~0.1-0.5 KB per object
    // - Narrowphase caches: ~0.5 KB per collision pair
    // - Scene/island management: ~4-16 KB
    // - Solver scratch memory: ~1-4 MB
    
    u64 ram = 0;
    ram += physics.body_count * 2048;        // 2 KB per body
    ram += physics.shape_count * 1024;       // 1 KB per shape
    ram += physics.constraint_count * 4096;  // 4 KB per constraint
    ram += physics.collision_pair_count * 512; // 512 bytes per pair
    ram += 4 * 1024 * 1024;                  // 4 MB solver scratch
    ram += physics.body_count * 256;         // Broadphase
    
    return ram;
}

void PhysicsAnalyzer::detect_physics_engine(PhysicsInfo& physics, StringView content) const {
    if (content.find("PhysX") != String::npos || content.find("PxPhysics") != String::npos || 
        content.find("physx") != String::npos) {
        physics.physics_engine = "PhysX";
        physics.engine_hint = "PhysX";
    } else if (content.find("Havok") != String::npos || content.find("hkp") != String::npos) {
        physics.physics_engine = "Havok";
        physics.engine_hint = "Havok";
    } else if (content.find("Bullet") != String::npos || content.find("btBullet") != String::npos ||
               content.find("btCollision") != String::npos || content.find("btDynamics") != String::npos) {
        physics.physics_engine = "Bullet";
        physics.engine_hint = "Bullet";
    } else if (content.find("ODE") != String::npos || content.find("dWorld") != String::npos ||
               content.find("dBody") != String::npos || content.find("dGeom") != String::npos) {
        physics.physics_engine = "ODE";
        physics.engine_hint = "ODE";
    } else if (content.find("Newton") != String::npos || content.find("NewtonBody") != String::npos) {
        physics.physics_engine = "Newton";
        physics.engine_hint = "Newton";
    }
}

String PhysicsAnalyzer::generate_report() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "Physics Analysis Report\n";
    ss << "======================\n";
    ss << "Total Scenes: " << stats_.total_scenes << "\n";
    ss << "Total Bodies: " << StringUtils::format_number(stats_.total_bodies) << "\n";
    ss << "Total Shapes: " << StringUtils::format_number(stats_.total_shapes) << "\n";
    ss << "Total Constraints: " << StringUtils::format_number(stats_.total_constraints) << "\n";
    ss << "Total Disk Size: " << StringUtils::format_bytes(stats_.total_disk_size) << "\n";
    ss << "Estimated RAM: " << StringUtils::format_bytes(stats_.total_estimated_ram) << "\n";
    ss << "Max Bodies/Scene: " << stats_.max_bodies_per_scene << "\n\n";
    
    if (!stats_.format_counts.empty()) {
        ss << "Format Distribution:\n";
        for (const auto& [fmt, count] : stats_.format_counts) {
            ss << "  " << fmt << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.engine_counts.empty()) {
        ss << "Physics Engine Detection:\n";
        for (const auto& [engine, count] : stats_.engine_counts) {
            ss << "  " << engine << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.shape_type_counts.empty()) {
        ss << "Shape Type Distribution:\n";
        for (const auto& [shape, count] : stats_.shape_type_counts) {
            ss << "  " << shape << ": " << count << "\n";
        }
    }
    
return ss.str();
}

String PhysicsStats::summary() const {
    std::stringstream ss;
    ss << "Physics Scenes: " << total_scenes 
       << ", Bodies: " << StringUtils::format_number(total_bodies)
       << ", Shapes: " << StringUtils::format_number(total_shapes)
       << ", Constraints: " << StringUtils::format_number(total_constraints)
       << ", Disk: " << StringUtils::format_bytes(total_disk_size)
       << ", RAM: " << StringUtils::format_bytes(total_estimated_ram);
    return ss.str();
}

} // namespace game_req