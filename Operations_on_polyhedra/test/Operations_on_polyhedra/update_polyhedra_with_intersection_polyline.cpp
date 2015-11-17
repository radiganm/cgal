#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/Timer.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

#define CGAL_TODO_WARNINGS

#include <CGAL/intersection_of_Polyhedra_3.h>
#include <CGAL/intersection_of_Polyhedra_3_refinement_visitor.h>


template <class Refs>
struct Halfedge_with_mark : public CGAL::HalfedgeDS_halfedge_base<Refs> {
  Halfedge_with_mark()
    : CGAL::HalfedgeDS_halfedge_base<Refs>(), m(false)
  {}

  bool m;   // A boundary marker for faces with different status
  void set_mark(bool v) 
  {
    m = v;
  }
};

template<class Polyhedron>
struct Edge_mark_property_map{
  typedef bool value_type;
  typedef value_type reference;
  typedef std::pair<typename Polyhedron::Halfedge_handle,Polyhedron*> key_type;
  typedef boost::read_write_property_map_tag category;

  friend reference get(Edge_mark_property_map,const key_type& key) {return key.first->m;}
  friend void put(Edge_mark_property_map,key_type key,value_type v) {key.first->m=v;}
};

// An items type using my halfedge.
struct Items_with_mark_on_hedge : public CGAL::Polyhedron_items_3 
{
  template <class Refs, class Traits>
  struct Halfedge_wrapper {
      typedef Halfedge_with_mark<Refs> Halfedge;
  };
};

typedef CGAL::Exact_predicates_inexact_constructions_kernel  Kernel;
typedef CGAL::Polyhedron_3<Kernel,Items_with_mark_on_hedge> Polyhedron;
typedef Edge_mark_property_map<Polyhedron> Edge_ppmap;
typedef CGAL::Node_visitor_refine_polyhedra<Polyhedron,CGAL::Default,Kernel,Edge_ppmap>      Split_visitor;
typedef std::vector<Kernel::Point_3> Polyline_3;

struct Is_not_marked{
  bool operator()(Polyhedron::Halfedge_const_handle h) const{
    return !h->m;
  }
};


int main(int argc,char** argv) {

    if (argc!=3){
      std::cerr << "Usage "<< argv[0] << " file1.off file2.off\n";
      return 1;
    }

    Polyhedron P, Q;    
    std::ifstream file(argv[1]);
    file >> P;
    file.close();
    file.open(argv[2]);
    file >> Q;
    file.close();
    
    CGAL::set_pretty_mode(std::cerr);

    std::cout << P.size_of_facets() << " " << Q.size_of_facets() << std::endl;
  
    std::list<Polyline_3> polylines;
    
    Split_visitor visitor;
    CGAL::Intersection_of_Polyhedra_3<Polyhedron,Kernel,Split_visitor> polyline_intersections(visitor);
    std::cout << "Vertices before " <<  P.size_of_vertices() << " " << Q.size_of_vertices() << std::endl;
    polyline_intersections( P,Q,std::back_inserter(polylines));
    std::cout << "Vertices after " <<  P.size_of_vertices() << " " << Q.size_of_vertices() << std::endl;
    
    std::cout << polylines.size() << " polylines" << std::endl;

    CGAL_assertion(P.is_valid());
    CGAL_assertion(Q.is_valid());
    
    std::ofstream out1("out1.off");
    std::ofstream out2("out2.off");
    out1 << P;
    out2 << Q;

    Is_not_marked criterium;
    std::list<Polyhedron> decomposition;
    CGAL::internal::corefinement::extract_connected_components(Q,criterium,std::back_inserter(decomposition));
    
    std::cout << "Q is decomposed in " << decomposition.size() << " pieces\n";
    
    int i=0;
    std::string prefix="Q_";
    std::string suffix=".off";
    for ( std::list<Polyhedron>::iterator it=decomposition.begin();it!=decomposition.end();++it)
    {
      std::stringstream ss;
      ss << prefix << i++ << suffix;
      std::ofstream out (ss.str().c_str());
      out << *it;
    }
    
    return 0;
}
