#define TINYOBJLOADER_IMPLEMENTATION

#include "ModelLoader.h"
#include <iostream>
#include <unordered_map>

bool ModelLoader::load(const std::string &filename, bool triangulate) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::string warn, err;

    tinyobj::ObjReaderConfig cfg;
    cfg.triangulate = triangulate; // quads → tris
    cfg.mtl_search_path = ""; // .mtl 동일 디렉토리

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filename, cfg)) {
        std::cerr << "TinyObjReader: " << reader.Error() << "\n";
        return false;
    }
    warn = reader.Warning();
    if (!warn.empty()) std::cerr << "[tinyobj warn] " << warn << "\n";

    attrib = reader.GetAttrib();
    shapes = reader.GetShapes();
    materials_ = reader.GetMaterials(); // 재질 정보 저장

    vertices_.clear();
    indices_.clear();
    faceMatIds_.clear();

    std::unordered_map<Vertex, uint32_t> unique; // Vertex dedup → index

    for (const auto &shape: shapes) {
        size_t index_offset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f]; // == 3

            // ---------- ① face 노멀 먼저 계산 ----------
            //  (노멀이 이미 들어있으면 나중에 덮어쓰지 않음)
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

            // ---------- ② 삼각형의 세 버텍스를 처리 ----------
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
                    nrm = faceNrm; // ★ 직접 계산한 노멀
                }

                glm::vec2 tex{};
                if (idx.texcoord_index >= 0) {
                    tex = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        attrib.texcoords[2 * idx.texcoord_index + 1]
                    };
                }

                Vertex vertex{pos, nrm, tex};

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

    glm::vec3 bboxMin( 1e9), bboxMax(-1e9);
    for (auto& v : vertices_) {
        bboxMin = glm::min(bboxMin, v.position);
        bboxMax = glm::max(bboxMax, v.position);
    }
    glm::vec3 center     = (bboxMin + bboxMax) * 0.5f;

    center_    = (bboxMin + bboxMax) * 0.5f;
    glm::vec3 diff = bboxMax - bboxMin;
    maxExtent_ = std::max({diff.x, diff.y, diff.z});
    return true;
}
