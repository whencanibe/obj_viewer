#define TINYOBJLOADER_IMPLEMENTATION

#include "ModelLoader.h"
#include <iostream>
#include <unordered_map>

bool ModelLoader::load(const std::string &filename, bool triangulate) {
    // attrib : 전체 vertex/normal/texcoord 배열이 저장됨.
    tinyobj::attrib_t attrib;
    // shapes : 각각의 mesh(face 그룹) 데이터가 들어 있음.
    std::vector<tinyobj::shape_t> shapes;
    std::string warn, err;

    tinyobj::ObjReaderConfig cfg;
    // quad , polygon 을 삼각형으로 변환할 지 여부
    cfg.triangulate = triangulate;
    cfg.mtl_search_path = ""; // .mtl 동일 디렉토리

    // Parse
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filename, cfg)) {
        std::cerr << "TinyObjReader: " << reader.Error() << "\n";
        return false;
    }
    warn = reader.Warning();
    if (!warn.empty()) std::cerr << "[tinyobj warn] " << warn << "\n";

    attrib = reader.GetAttrib();
    shapes = reader.GetShapes();
    materials_ = reader.GetMaterials();

    vertices_.clear();
    indices_.clear();
    faceMatIds_.clear();

    std::unordered_map<Vertex, uint32_t> unique; // 동일한 Vertex 중복 저장을 막고 index 기반 렌더링을 위한 맵.

    for (const auto &shape: shapes) {
        size_t index_offset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f]; // == 3

            // Normal이 없을 경우를 대비해 직접 계산 (노멀이 이미 들어있으면 나중에 덮어쓰지 않음)
            glm::vec3 faceNrm(0.0f); {
                tinyobj::index_t idx0 = shape.mesh.indices[index_offset + 0];
                tinyobj::index_t idx1 = shape.mesh.indices[index_offset + 1];
                tinyobj::index_t idx2 = shape.mesh.indices[index_offset + 2];

                glm::vec3 p0 = {
                    attrib.vertices[3 * idx0.vertex_index + 0],
                    attrib.vertices[3 * idx0.vertex_index + 1],
                    attrib.vertices[3 * idx0.vertex_index + 2]
                };
                glm::vec3 p1 = {
                    attrib.vertices[3 * idx1.vertex_index + 0],
                    attrib.vertices[3 * idx1.vertex_index + 1],
                    attrib.vertices[3 * idx1.vertex_index + 2]
                };
                glm::vec3 p2 = {
                    attrib.vertices[3 * idx2.vertex_index + 0],
                    attrib.vertices[3 * idx2.vertex_index + 1],
                    attrib.vertices[3 * idx2.vertex_index + 2]
                };

                faceNrm = glm::normalize(glm::cross(p1 - p0, p2 - p0));
            }

            // face의 각 vertex 처리
            for (int v = 0; v < fv; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                glm::vec3 pos = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };

                // 노멀이 있으면 그대로, 없으면 faceNrm 사용
                glm::vec3 nrm{};
                if (idx.normal_index >= 0) {
                    nrm = {
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]
                    };
                } else {
                    nrm = faceNrm; // 직접 계산한 노멀
                }

                glm::vec2 tex{};
                if (idx.texcoord_index >= 0) {
                    tex = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        attrib.texcoords[2 * idx.texcoord_index + 1]
                    };
                }

                // Vertex 중복 제거 및 저장
                Vertex vertex{pos, nrm, tex};
                // 기존 vertex 못찾으면 .end() 값 반환
                if (auto it = unique.find(vertex); it != unique.end()) {
                    indices_.push_back(it->second);
                } else {
                    uint32_t newIndex = static_cast<uint32_t>(vertices_.size());
                    unique[vertex] = newIndex;
                    vertices_.push_back(vertex);
                    indices_.push_back(newIndex);
                }
            }

            index_offset += fv;
        }
    }

    // AABB Bounding box 계산
    glm::vec3 bboxMin(1e9), bboxMax(-1e9);
    for (auto &v: vertices_) {
        bboxMin = glm::min(bboxMin, v.position);
        bboxMax = glm::max(bboxMax, v.position);
    }

    center_ = (bboxMin + bboxMax) * 0.5f;
    glm::vec3 diff = bboxMax - bboxMin;
    maxExtent_ = std::max({diff.x, diff.y, diff.z});

    return true;
}
