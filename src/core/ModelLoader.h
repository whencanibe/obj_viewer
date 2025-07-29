#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

enum class NormalMode { Vertex, Face };

/// 단일 정점 구조 – 인덱스 기반으로 묶어서 사용
struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 texcoord{};

    bool operator==(const Vertex &rhs) const {
        return position == rhs.position &&
               normal == rhs.normal &&
               texcoord == rhs.texcoord;
    }
};

/// Vertex를 key로 사용하기 위한 해시
namespace std {
    template<>
    struct hash<Vertex> {
        size_t operator()(const Vertex &v) const noexcept {
            auto h1 = hash<float>()(v.position.x) ^ (hash<float>()(v.position.y) << 1);
            auto h2 = hash<float>()(v.position.z) ^ (hash<float>()(v.normal.x) << 1);
            auto h3 = hash<float>()(v.normal.y) ^ (hash<float>()(v.normal.z) << 1);
            auto h4 = hash<float>()(v.texcoord.x) ^ (hash<float>()(v.texcoord.y) << 1);
            return h1 ^ h2 ^ h3 ^ h4;
        }
    };
}

class ModelLoader {
public:
    bool load(const std::string &filename, bool triangulate = true);

    NormalMode      normalMode()  const { return mode_; }
    void            setNormalMode(NormalMode m);   // face ↔ vertex 토글

    const std::vector<Vertex> &vertices() const { return vertices_; }
    const std::vector<uint32_t> &indices() const { return indices_; }
    const std::vector<tinyobj::material_t> &materials() const { return materials_; }
    const glm::vec3 &center() const { return center_; }
    float maxExtent() const { return maxExtent_; }

    /// face(i) 가 어떤 material id를 쓰는지 (indices는 3‑배수 단위)
    const std::vector<int> &materialIdsPerFace() const { return faceMatIds_; }

private:
    void rebuildVertices();

    std::vector<glm::vec3> rawPos_;          // 위치만
    std::vector<uint32_t>  rawIdx_;          // v1,v2,v3, .. (삼각형 인덱스)
    std::vector<glm::vec3> faceNrm_;         // 삼각형당 1개
    std::vector<glm::vec3> vertNrm_;         // 버텍스당 1개(평균)

    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;

    std::vector<tinyobj::material_t> materials_;
    std::vector<int> faceMatIds_; // face(삼각형) 단위 material id

    NormalMode mode_ = NormalMode::Vertex;   // 현재 모드
    glm::vec3 center_{};
    float maxExtent_ = 1.0f;
};




#endif //MODELLOADER_H
