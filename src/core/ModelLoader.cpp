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

    rawPos_.clear();
    rawIdx_.clear();
    faceNrm_.clear();

    size_t vertexCount = attrib.vertices.size() / 3;
    rawPos_.reserve(vertexCount);
    vertNrm_.assign(vertexCount, glm::vec3(0.0f));   // 평균노멀 누적용

    vertices_.clear();
    indices_.clear();
    faceMatIds_.clear();

    // rawPos_ 채우기
    for (size_t i = 0; i < vertexCount; ++i) {
        glm::vec3 p(attrib.vertices[3*i+0],
                    attrib.vertices[3*i+1],
                    attrib.vertices[3*i+2]);
        rawPos_.push_back(p);
    }

    // 삼각형 루프: 인덱스,면노멀,버텍스노멀 누적
    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f];   // triangulate=true → 3
            tinyobj::index_t idx0 = shape.mesh.indices[index_offset+0];
            tinyobj::index_t idx1 = shape.mesh.indices[index_offset+1];
            tinyobj::index_t idx2 = shape.mesh.indices[index_offset+2];

            // 인덱스 배열
            rawIdx_.push_back(idx0.vertex_index);
            rawIdx_.push_back(idx1.vertex_index);
            rawIdx_.push_back(idx2.vertex_index);

            // 면 노멀
            glm::vec3 p0 = rawPos_[idx0.vertex_index];
            glm::vec3 p1 = rawPos_[idx1.vertex_index];
            glm::vec3 p2 = rawPos_[idx2.vertex_index];
            glm::vec3 faceN = glm::normalize(glm::cross(p1-p0, p2-p0));
            faceNrm_.push_back(faceN);

            // 버텍스 노멀 누적
            auto accumulate = [&](tinyobj::index_t idx){
                if (idx.normal_index >= 0) {
                    vertNrm_[idx.vertex_index] += glm::vec3(
                        attrib.normals[3*idx.normal_index+0],
                        attrib.normals[3*idx.normal_index+1],
                        attrib.normals[3*idx.normal_index+2]);
                } else {
                    vertNrm_[idx.vertex_index] += faceN;   // 노멀 없으면 면노멀
                }
            };
            accumulate(idx0); accumulate(idx1); accumulate(idx2);

            index_offset += fv;
        }
    }

    // 버텍스 평균노멀 정규화
    for (auto& n : vertNrm_) n = glm::normalize(n);

    rebuildVertices();

    // AABB Bounding box 계산
    glm::vec3 bboxMin(1e9), bboxMax(-1e9);
    for (auto& p : rawPos_) {
        bboxMin = glm::min(bboxMin, p);
        bboxMax = glm::max(bboxMax, p);
    }

    center_ = (bboxMin + bboxMax) * 0.5f;
    glm::vec3 diff = bboxMax - bboxMin;
    maxExtent_ = std::max({diff.x, diff.y, diff.z});

    return true;
}

void ModelLoader::rebuildVertices()
{
    vertices_.clear(); indices_.clear();
    vertices_.reserve(rawIdx_.size());

    std::unordered_map<Vertex,uint32_t> uniq; // 동일한 Vertex 중복 저장을 막고 index 기반 렌더링을 위한 맵.
    Vertex v;

    for(size_t i=0;i<rawIdx_.size();++i){
        uint32_t vid = rawIdx_[i];
        v.position   = rawPos_[vid];
        v.texcoord   = {};                      // (필요하면 채움)
        v.normal     = (mode_==NormalMode::Face)
                       ? faceNrm_[i/3]          // 삼각형 노멀
                       : vertNrm_[vid];         // 평균 노멀

        auto it = uniq.find(v);
        if(it==uniq.end()){
            uint32_t newId = vertices_.size();
            uniq[v]=newId;
            vertices_.push_back(v);
            indices_.push_back(newId);
        }else
            indices_.push_back(it->second);
    }
}

void ModelLoader::setNormalMode(NormalMode m)
{
    if (mode_ == m) return;
    mode_ = m;
    rebuildVertices();          // CPU 배열 갱신
}
