#ifndef CGAL_DATA_CLASSIFICATION_SEGMENTATION_ATTRIBUTE_VERTICALITY_H
#define CGAL_DATA_CLASSIFICATION_SEGMENTATION_ATTRIBUTE_VERTICALITY_H

#include <vector>

#include <CGAL/Data_classification/Local_eigen_analysis.h>

namespace CGAL {

  /*!
    \ingroup PkgDataClassification

    \brief Segmentation attribute based on local verticality.

    The orientation of the best fitting plane of a local neighborhood
    of the considered point can be useful to discriminate facades from
    the ground.

    \tparam Kernel The geometric kernel used.
    \tparam RandomAccessIterator Iterator over the input.
    \tparam PointPMap Property map to access the input points.
    \tparam DiagonalizeTraits Solver used for matrix diagonalization.
  */
template <typename Kernel, typename RandomAccessIterator, typename PointPMap,
          typename DiagonalizeTraits = CGAL::Default_diagonalize_traits<double,3> >
class Segmentation_attribute_verticality : public Segmentation_attribute
{
  typedef Data_classification::Local_eigen_analysis<Kernel, RandomAccessIterator,
                                                    PointPMap, DiagonalizeTraits> Local_eigen_analysis;
  std::vector<double> verticality_attribute;
  
public:
  /*!
    \brief Constructs the attribute using local eigen analysis.

    \param begin Iterator to the first input object
    \param end Past-the-end iterator
    \param eigen Class with precompute eigenvectors and eigenvalues
    \param weight Weight of the attribute
  */
  Segmentation_attribute_verticality (RandomAccessIterator begin,
                                      RandomAccessIterator end,
                                      const Local_eigen_analysis& eigen,
                                      double weight = 1.)
  {
    this->weight = weight;
    typename Kernel::Vector_3 vertical (0., 0., 1.);

    for (std::size_t i = 0; i < (std::size_t)(end - begin); i++)
      {
        typename Kernel::Vector_3 normal = eigen.normal_vector(i);
        normal = normal / std::sqrt (normal * normal);
        verticality_attribute.push_back (1. - std::fabs(normal * vertical));
      }
    
    this->compute_mean_max (verticality_attribute, this->mean, this->max);
    //    max *= 2;
  }

  /*!
    \brief Constructs the attribute using provided normals of points.

    \tparam NormalPMap Property map to access the normal vectors of the input points.
    \param begin Iterator to the first input object
    \param end Past-the-end iterator
    \param weight Weight of the attribute
  */
  template <typename NormalPMap>
  Segmentation_attribute_verticality (const RandomAccessIterator& begin,
                                      const RandomAccessIterator& end,
                                      NormalPMap normal_pmap,
                                      double weight = 1.)
  {
    this->weight = weight;
    typename Kernel::Vector_3 vertical (0., 0., 1.);

    for (std::size_t i = 0; i < (std::size_t)(end - begin); i++)
      {
        typename Kernel::Vector_3 normal = get(normal_pmap, begin[i]);
        normal = normal / std::sqrt (normal * normal);
        verticality_attribute.push_back (1. - std::fabs(normal * vertical));
      }
    
    this->compute_mean_max (verticality_attribute, this->mean, this->max);
    //    max *= 2;
  }


  /// \cond SKIP_IN_MANUAL
  virtual double value (std::size_t pt_index)
  {
    return verticality_attribute[pt_index];
  }

  virtual std::string id() { return "verticality"; }
  /// \endcond
};


}

#endif // CGAL_DATA_CLASSIFICATION_SEGMENTATION_ATTRIBUTE_VERTICALITY_H
