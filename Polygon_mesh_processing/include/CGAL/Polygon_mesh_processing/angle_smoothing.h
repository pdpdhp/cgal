#ifndef ANGLE_SMOOTHING_H
#define ANGLE_SMOOTHING_H
//#define CGAL_ANGLE_BASE_SMOOTHING_DEBUG

#include <CGAL/Polygon_mesh_processing/compute_normal.h>

#include <CGAL/Polygon_mesh_processing/internal/named_function_params.h>
#include <CGAL/Polygon_mesh_processing/internal/named_params_helper.h>


namespace CGAL {

namespace Polygon_mesh_processing {





template<typename PolygonMesh, typename VertexPointMap, typename GeomTraits>
class Angle_remesher
{


    typedef typename boost::graph_traits<PolygonMesh>::halfedge_descriptor halfedge_descriptor;
    typedef typename boost::graph_traits<PolygonMesh>::vertex_descriptor vertex_descriptor;
    typedef CGAL::Halfedge_around_source_iterator<PolygonMesh> halfedge_around_source_iterator;


    typedef typename GeomTraits::Point_3 Point;
    typedef typename GeomTraits::Vector_3 Vector;
    typedef typename GeomTraits::Segment_3 Segment;



    // gather incident lines to a map
    // <one halfedge around v, pair of incident halfedges to this halfedge around v>
    typedef std::pair<halfedge_descriptor, halfedge_descriptor> He_pair;
    typedef std::map<halfedge_descriptor, He_pair> Edges_around_map;


public:
    Angle_remesher(PolygonMesh& pmesh, VertexPointMap& vpmap) : mesh_(pmesh), vpmap_(vpmap)
    {}




    void angle_relaxation()
    {

        std::map<vertex_descriptor, Point> barycenters;
        boost::vector_property_map<Vector> n_map;

        for(vertex_descriptor v : vertices(mesh_))
        {

            if(!is_border(v, mesh_))
            {
                #ifdef CGAL_ANGLE_BASE_SMOOTHING_DEBUG
                std::cout<<"processing vertex: "<< v << std::endl;
                #endif


                // compute normal to v
                Vector vn = compute_vertex_normal(v, mesh_,
                                                  Polygon_mesh_processing::parameters::vertex_point_map(vpmap_)
                                                  .geom_traits(GeomTraits()));
                n_map[v] = vn;



                Edges_around_map he_map;
                typename Edges_around_map::iterator it;

                for(halfedge_descriptor hi : halfedges_around_source(v, mesh_)) // or make it around target
                    he_map[hi] = He_pair(next(hi, mesh_), prev(opposite(hi, mesh_) ,mesh_));



                #ifdef CGAL_ANGLE_BASE_SMOOTHING_DEBUG
                for(it = he_map.begin(); it!=he_map.end(); ++it)
                {
                    halfedge_descriptor main_he = it->first;
                    He_pair he_pair = it->second;
                    std::cout<< "main: " << main_he;
                    std::cout<<" ("<<source(main_he, mesh_)<<"->"<< target(main_he, mesh_)<<")";
                    std::cout<< " - incident: "<< he_pair.first;
                    std::cout<<" ("<<source(he_pair.first, mesh_)<<"->"<< target(he_pair.first, mesh_)<<")";
                    std::cout<<" and " << he_pair.second;
                    std::cout<<" ("<<source(he_pair.second, mesh_)<<"->"<< target(he_pair.second, mesh_)<<")"<<std::endl;
                }
                #endif


                // calculate movement
                Vector move = CGAL::NULL_VECTOR;

                for(it = he_map.begin(); it != he_map.end(); ++it)
                {
                    halfedge_descriptor main_he = it->first;
                    He_pair incident_pair = it->second;

                    Vector rotated_edge = rotate_edge(main_he, incident_pair);

                    move += rotated_edge;

                }

                barycenters[v] = get(vpmap_, v) + (move / (double)he_map.size());


            } // not on border
        } // for each v



        // compute locations on tangent plane
        typedef typename std::map<vertex_descriptor, Point>::value_type VP;
        std::map<vertex_descriptor, Point> new_locations;
        for(const VP& vp: barycenters)
        {
            Point q = vp.second;
            Point p = get(vpmap_, vp.first);
            Vector n = n_map[vp.first];

            new_locations[vp.first] = q + ( n * Vector(q, p) ) * n ;
        }


        // perform moves
        for(const VP& vp : new_locations)
        {
            #ifdef CGAL_ANGLE_BASE_SMOOTHING_DEBUG
            std::cout << "from: "<< get(vpmap_, vp.first);
            #endif

            put(vpmap_, vp.first, vp.second);

            #ifdef CGAL_ANGLE_BASE_SMOOTHING_DEBUG
            std::cout<<" moved at: "<< vp.second << std::endl;
            #endif
        }




    }






private:
    PolygonMesh& mesh_;
    VertexPointMap& vpmap_;



    double sqlength(const vertex_descriptor& v1, const vertex_descriptor& v2) const
    {
        return to_double(CGAL::squared_distance(get(vpmap_, v1), get(vpmap_, v2)));
    }

    double sqlength(const halfedge_descriptor& h) const
    {
      vertex_descriptor v1 = target(h, mesh_);
      vertex_descriptor v2 = source(h, mesh_);
      return sqlength(v1, v2);
    }


    Vector rotate_edge(const halfedge_descriptor& main_he, const He_pair& incd_edges)
    {

        // get common vertex around which the edge is rotated
        Point s = get(vpmap_, target(main_he, mesh_));

        // pv is the vertex that is being moved
        Point pv = get(vpmap_, source(main_he, mesh_));

        // get "equidistant" points - in fact they are at equal angles
        Point equidistant_p1 = get(vpmap_, target(incd_edges.first, mesh_));
        Point equidistant_p2 = get(vpmap_, source(incd_edges.second, mesh_));

        Vector edge1(s, equidistant_p1);
        Vector edge2(s, equidistant_p2);
        Vector s_pv(s, pv);


        // check degenerate cases
        double tolerance = 1e-5; // to think about it

        if ( edge1.squared_length()          < tolerance ||
             edge2.squared_length()          < tolerance ||
             sqlength(main_he)               < tolerance ||
             (edge1 - s_pv).squared_length() < tolerance ||
             (edge2 - s_pv).squared_length() < tolerance   )
        {
            return CGAL::NULL_VECTOR;
        }

        CGAL_assertion(s_pv.squared_length() > tolerance);

        // get bisector
        Vector bisector = CGAL::NULL_VECTOR;
        internal::normalize(edge1, GeomTraits());
        internal::normalize(edge2, GeomTraits());
        bisector = edge1 + edge2;


        // under 2 degrees deviation consider it flat
        if( bisector.squared_length() < 0.001 ) // sin(theta) = 0.0316 => theta = 1.83
        {

            // angle is (almost) 180 degrees, take the perpendicular
            Vector normal_vec = find_perpendicular(edge1, s, pv); // normal to edge and found on (s-pv)'s plane

            CGAL_assertion(normal_vec != CGAL::NULL_VECTOR);
            CGAL_assertion(CGAL::scalar_product(edge1, normal_vec) < tolerance);

            Segment b_segment(s, s + normal_vec);
            Point b_segment_end = b_segment.target();

            if(CGAL::angle(b_segment_end, s, pv) == CGAL::OBTUSE)
            {
                b_segment = b_segment.opposite();
            }

            bisector = Vector(b_segment);

        }


        correct_bisector(bisector, main_he);


        double target_length = CGAL::sqrt(sqlength(main_he));
        double bisector_length = CGAL::sqrt(bisector.squared_length());



        CGAL_assertion(   ( target_length - tolerance    <   bisector_length     ) &&
                          ( bisector_length        <   target_length + tolerance )    );


        return bisector;


    }


    Vector find_perpendicular(const Vector& input_vec, const Point& s, const Point& pv)
    {
        Vector s_pv(s, pv);
        Vector aux_normal = CGAL::cross_product(input_vec, s_pv);

        return CGAL::cross_product(aux_normal, input_vec);

    }

    Vector max_vector(const Vector& vec1, const Vector& vec2)
    {
        if (vec1.squared_length() > vec2.squared_length())
            return vec1;
        else
            return vec2;
    }

    void correct_bisector(Vector& bisector_vec, const halfedge_descriptor& main_he)
    {

        // get common vertex around which the edge is rotated
        Point s = get(vpmap_, target(main_he, mesh_));

        // create a segment to be able to translate
        Segment bisector(s, s + bisector_vec);

        // scale
        double scale_factor = CGAL::sqrt(  sqlength(main_he) / bisector.squared_length() );
        typename GeomTraits::Aff_transformation_3 t_scale(CGAL::SCALING, scale_factor);
        bisector = bisector.transform(t_scale);

        // translate
        Vector vec(bisector.source(), s);
        typename GeomTraits::Aff_transformation_3 t_translate(CGAL::TRANSLATION, vec);
        bisector = bisector.transform(t_translate);

        // take the opposite so that their sum is the overall displacement
        bisector_vec = -Vector(bisector);

    }






};









template<typename PolygonMesh, typename NamedParameters>
void angle_remeshing(PolygonMesh& pmesh, const NamedParameters& np)
{



    //CGAL_PMP_NP_CLASS np;
    //np = CGAL::Polygon_mesh_processing::parameters::all_default();

    using boost::choose_param;
    using boost::get_param;

    typedef typename GetVertexPointMap<PolygonMesh, NamedParameters>::const_type VertexPointMap;
    VertexPointMap vpmap = choose_param(get_param(np, internal_np::vertex_point),
                                 get_const_property_map(CGAL::vertex_point, pmesh));


    typedef typename GetGeomTraits<PolygonMesh, NamedParameters>::type Traits;


    CGAL::Polygon_mesh_processing::Angle_remesher<PolygonMesh, VertexPointMap, Traits> remesher(pmesh, vpmap);
    remesher.angle_relaxation();




}











} // namespace Polygon_mesh_processing
} //namespace CGAL






#endif // ANGLE_SMOOTHING_H

