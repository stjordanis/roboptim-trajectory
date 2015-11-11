#ifndef ROBOPTIM_TRAJECTORY_CONSTRAINTS_OVER_SPLINES_HH
# define ROBOPTIM_TRAJECTORY_CONSTRAINTS_OVER_SPLINES_HH

# include <vector>

# include <boost/shared_ptr.hpp>

# include <roboptim/core/differentiable-function.hh>

namespace roboptim
{
  namespace trajectory
  {
    /// \brief Constraint function on a spline's interval
    ///
    /// \tparam T Matrix type
    /// \tparam S Spline type
    template <typename T, typename S>
    class ConstraintsOverSplines : public roboptim::GenericDifferentiableFunction<T>
    {
    public:
      ROBOPTIM_DIFFERENTIABLE_FUNCTION_FWD_TYPEDEFS_
        (roboptim::GenericDifferentiableFunction<T>);

      typedef typename S::polynomial_t polynomial_t;
      typedef Function::interval_t interval_t;
      typedef std::vector<polynomial_t> polynomials_t;
      typedef boost::shared_ptr<S> spline_t;
      typedef std::vector<boost::shared_ptr<S> > splines_t;

      ConstraintsOverSplines (const splines_t& splines, size_t splineIdx,
           unsigned int order, value_type startingPoint,
           size_type inputSize);

    protected:

      static std::string GenerateName (const splines_t& splines, size_t splineIdx,
           unsigned int order, value_type startingPoint);

      static interval_t ComputeInterval (const splines_t& splines, size_t splineIdx,
           value_type startingPoint);

      static size_t ComputeIntervalIdx (const splines_t& splines, size_t splineIdx,
           value_type startingPoint);

      static size_type ComputeStartIdx (const splines_t& splines, size_t splineIdx);

      void update(const_argument_ref x) const;

      virtual void impl_compute(result_ref result, const_argument_ref x) const;

      virtual void impl_gradient(gradient_ref grad, const_argument_ref x, size_type i) const;

      /// \brief Dimension of the spline
      size_t dimension_;

      /// \brief Time range of the constraint
      typename S::interval_t interval_;

      /// \brief Index of the influencing control Points
      size_type startingIndex_;

      /// \brief Polynomial used to compute the minimum and maximum
      mutable polynomial_t p_;

      /// \brief Coefficients of the polynomial
      polynomials_t coefs_;
    };
  }
}

#include <roboptim/trajectory/constraints-over-splines.hxx>

#endif //! ROBOPTIM_TRAJECTORY_CONSTRAINTS_OVER_SPLINES_HH
