#pragma once

#include <vector>
#include <memory>
#include "device_array.h"
#include "ldpMat\ldp_basic_mat.h"
#include <map>
#include <set>
#include "definations.h"
#include "graph\AbstractGraphObject.h"
#include "CudaBsrMatrix.h"
#ifndef __CUDACC__
#include <eigen\Dense>
#include <eigen\Sparse>
#endif

namespace arcsim
{
	class ArcSimManager;
}

namespace ldp
{
	class ClothManager;
	class LevelSet3D;
	class GpuSim
	{
	public:
		//		 c
		//		 /\
		//		/  \
		//	  a ---- b
		//		\  /
		//		 \/
		//		 d
		// edge:	 ab
		// bendEdge: cd
		// faceIdx: abc, bad
		// _idxWorld: index in world space
		// _idxTex: index in tex space
		struct EdgeData
		{
			ldp::Int2 edge_idxWorld;
			ldp::Int2 faceIdx;
		};
	public:
		GpuSim();
		~GpuSim();

		// init from cloth manager
		void init(ClothManager* clothManager);

		// init from arcsim
		void init(arcsim::ArcSimManager* arcSimManager);

		// perform simulation for one time-step
		void run_one_step();

		// reset cloths to the initial state
		void restart();

		// update the material parameters
		void updateMaterial();

		// update cloth topologies
		void updateTopology();

		// simulatio parameter update; resulting in numerical updates
		void updateNumeric();

		static __device__ __host__ inline int vertPair_to_idx(ldp::Int2 v, int n)
		{
			return v[0] * n + v[1];
		}
		static __device__ __host__ inline ldp::Int2 vertPair_from_idx(int idx, int n)
		{
			return ldp::Int2(idx/n, idx%n);
		}
	protected:
		void createMaterialMemory();
		void releaseMaterialMemory();

		void updateTopology_arcSim();
		void updateTopology_clothManager();

		void linearSolve();

	protected:
		void setup_sparse_structure_from_cpu();
	private:
		ClothManager* m_clothManager = nullptr;
		arcsim::ArcSimManager* m_arcSimManager = nullptr;
		cusparseHandle_t m_cusparseHandle = nullptr;

		ldp::LevelSet3D* m_bodyLvSet_h = nullptr;
		DeviceArray<float> m_bodyLvSet_d;

		std::vector<ldp::Int4> m_faces_idxWorld_h;				// triangle face with 1 paded int
		DeviceArray<ldp::Int4> m_faces_idxWorld_d;
		std::vector<ldp::Int4> m_faces_idxTex_h;
		DeviceArray<ldp::Int4> m_faces_idxTex_d;
		std::vector<int> m_faces_idxMat_h;						// material index of each face
		DeviceArray<cudaTextureObject_t> m_faces_texStretch_d;	// cuda texture of stretching
		DeviceArray<cudaTextureObject_t> m_faces_texBend_d;		// cuda texture of bending
		std::vector<EdgeData> m_edgeData_h;
		DeviceArray<EdgeData> m_edgeData_d;

		///////////////// precomputed data /////////////////////////////////////////////////////////////
		std::vector<float> m_edgeThetaIdeals_h;					// the folding angles (in arc) on the edge
		DeviceArray<float> m_edgeThetaIdeals_d;			

		std::vector<int> m_faceEdge_vertIds_h;	// computing internal forces, the vertex index of each face/edge/bend
		DeviceArray<int> m_faceEdge_vertIds_d;
		std::vector<int> m_faceEdge_order_h;	// the face/edge/bend filling order, beforScan_A[order]
		DeviceArray<int> m_faceEdge_order_d;
		DeviceArray<ldp::Mat3f> m_faceEdge_beforScan_A;
		DeviceArray<ldp::Float3> m_faceEdge_beforScan_b;
		///////////////// solve for the simulation linear system: A*dv=b////////////////////////////////
		std::shared_ptr<CudaBsrMatrix> m_A;
		DeviceArray<ldp::Float3> m_b;							
		DeviceArray<ldp::Float2> m_texCoord_init;				// material (tex) space vertex texCoord		
		DeviceArray<ldp::Float3> m_x_init;						// world space vertex position
		DeviceArray<ldp::Float3> m_x;							// position of current step	
		DeviceArray<ldp::Float3> m_v;							// velocity of current step
		DeviceArray<ldp::Float3> m_dv;							// velocity changed in this step
		
		//////////////////////// material related///////////////////////////////////////////////////////
	public:
		struct StretchingData { 
			enum {
				DIMS = 2,
				POINTS = 5
			};
			ldp::Float4 d[DIMS][POINTS];
		};
		struct StretchingSamples {
			enum{
				SAMPLES = 40
			};
			ldp::Float4 s[SAMPLES][SAMPLES][SAMPLES];
		};
		struct BendingData {
			enum {
				DIMS = 3,
				POINTS = 5
			};
			float d[DIMS][POINTS];
		};
		std::vector<StretchingSamples> m_stretchSamples_h;			
		std::vector<BendingData> m_bendingData_h;
		std::vector<cudaArray_t> m_stretchSamples_d;
		std::vector<DeviceArray2D<float>> m_bendingData_d;
		std::vector<cudaTextureObject_t> m_stretchSamples_tex_h;
		std::vector<cudaTextureObject_t> m_bendingData_tex_h;
	};
}