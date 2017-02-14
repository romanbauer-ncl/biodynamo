//=================================================================================================
#ifndef __BDM_FEB3_INTERFACE__
#define __BDM_FEB3_INTERFACE__
//=================================================================================================
#include <vector>
// BioDynamoc header files:
#include "backend.h"
#include "cell.h"
#include "daosoa.h"
#include "resource_manager.h"
#include "scheduler.h"
#include "timing.h"
//=================================================================================================
class Vector_3D {
public:
    // public member functions: constructor(s) & destructor
    Vector_3D () { comp[0]=comp[1]=comp[2]=0.0; }
    Vector_3D (double x, double y, double z) { comp[0]=x; comp[1]=y; comp[2]=z; }
    Vector_3D (const Vector_3D& v) { comp[0]=v.comp[0]; comp[1]=v.comp[1]; comp[2]=v.comp[2]; }
    ~Vector_3D () {}
public:
    // public member objects
    double comp[3];
};
//=================================================================================================
namespace bdm_feb3 {
//=================================================================================================
class ContinuousInterfaceData {
public:
    // public member functions: constructor(s) & destructor
    ContinuousInterfaceData (int n_cell_types =1) {
        const unsigned int n_vertices = 8;
        oxygen_level.assign(n_vertices, 0.0);
        oxygen_level_gradient.assign(n_vertices, Vector_3D());
        normoxic_cells.resize(n_vertices);
        hypoxic_cells.resize(n_vertices);
        for (unsigned int n=0; n<n_vertices; n++) {
            normoxic_cells[n].assign(n_cell_types, 0);
            hypoxic_cells[n].assign(n_cell_types, 0);
        }
    }
    ~ContinuousInterfaceData () {}
public:
    // public member objects: input/output data
    std::vector< double >    oxygen_level;
    std::vector< Vector_3D > oxygen_level_gradient;
    std::vector< std::vector<unsigned int> > normoxic_cells;
    std::vector< std::vector<unsigned int> > hypoxic_cells;
};
//-------------------------------------------------------------------------------------------------
class DiscontinuousInterfaceData {
public:
    // public member functions: constructor(s) & destructor
    DiscontinuousInterfaceData () {}
    ~DiscontinuousInterfaceData () {}
public:
    // public member objects: input/output data
    double    ecm_density;
    Vector_3D ecm_density_gradient;
};
//=================================================================================================
} // end of namespace: bdm_feb3
//=================================================================================================
// Enumeration of the 8 vertices of the BDM_CubicDomain as in the following schematic. Thus the 
// continuous interface data are distributed over the 8 points of the hexahedron, while the
// discontinuous interface data are assumed uniform (constant) in the cubic domain.
//
//        7        6
//        o--------o
//       /:       /|
//      / :      / |
//   4 /  :   5 /  |
//    o--------o   |
//    |   o....|...o 2
//    |  .3    |  /
//    | .      | /
//    |.       |/
//    o--------o
//    0        1
// 
//=================================================================================================
class BDM_CubicDomain {
public:
    // public member functions: constructor(s) & destructor
    BDM_CubicDomain () : _is_init(false) {}
    ~BDM_CubicDomain () {}
    // public member functions:
    void init (size_t ncell, std::vector<Vector_3D>& v) {
        _cells = bdm::daosoa< bdm::Cell >(ncell);
        if ( v.size() != 8 ) throw;
        else _vertex = v;
        _is_init = true;
    }
    inline bool is_init () const { return _is_init; }
private:
    // private member objects
    bool _is_init;
    bdm::daosoa< bdm::Cell > _cells;
    std::vector< Vector_3D > _vertex;
public:
    // public member objects
    bdm_feb3::ContinuousInterfaceData cont_fd;
    bdm_feb3::DiscontinuousInterfaceData disc_fd;
};
//=================================================================================================
#endif
//=================================================================================================
