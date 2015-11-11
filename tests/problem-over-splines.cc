// Copyright (C) 2015 by Félix Darricau, AIST, CNRS, EPITA.
//
// This file is part of the roboptim.
//
// roboptim is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// roboptim is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with roboptim.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <vector>
#include <sstream>

#include <boost/filesystem/fstream.hpp>

#include <roboptim/core/io.hh>

#include "shared-tests/fixture.hh"

#include <roboptim/core/function/constant.hh>

#include <roboptim/trajectory/cubic-b-spline.hh>
#include <roboptim/trajectory/b-spline.hh>
#include <roboptim/trajectory/jerk-over-splines-factory.hh>
#include <roboptim/trajectory/problem-over-splines-factory.hh>

#include <roboptim/trajectory/visualization/cubic-b-spline-matplotlib.hh>
#include <roboptim/trajectory/visualization/b-spline-matplotlib.hh>

#include <roboptim/trajectory/fwd.hh>

#include <roboptim/core/visualization/matplotlib-function.hh>

using namespace roboptim;
using namespace roboptim::visualization;
using namespace roboptim::visualization::matplotlib;
using namespace roboptim::trajectory;
using namespace roboptim::trajectory::visualization::matplotlib;

typedef boost::mpl::list< ::roboptim::trajectory::CubicBSpline,
			  ::roboptim::trajectory::BSpline<3> > splinesType_t;

template <typename T>
std::string splineName();

template <>
std::string splineName<roboptim::trajectory::CubicBSpline> ()
{
  return "cubic-b-spline";
}

template <>
std::string splineName<roboptim::trajectory::BSpline<3> > ()
{
  return "b-spline";
}

template <typename T, typename S>
void processResult (const typename Solver<T>::result_t& res,
    boost::shared_ptr<S>& spline,
    boost::shared_ptr<S>& spline2,
    Matplotlib& matplotlib,
    boost::filesystem::ofstream& pythonPlot)
{
  typedef typename GenericFunction<T>::value_type value_type;
  typedef Solver<T> solver_t;

  value_type step = 0.005;

  static int n = 1;

  std::stringstream ss;
  ss << " after optimization #" << (n++);
  const std::string title_end = ss.str ();

  switch (res.which ())
    {
    case solver_t::SOLVER_VALUE:
      {
        // Get the result.
        Result result = boost::get<Result> (res);

        spline->setParameters (result.x.segment (0, 13));
        spline2->setParameters (result.x.segment (13, 13));

	pythonPlot
	  << (matplotlib
	      << plot_spline (*spline, step)
	      << title ((spline->getName () + title_end).c_str ())
	      << plot_spline (*spline2, step)
	      << title ((spline2->getName () + title_end).c_str ())
	      ) << std::endl;

        std::cout << result << std::endl;
        break;
      }

    case solver_t::SOLVER_VALUE_WARNINGS:
      {
        // Get the result.
        ResultWithWarnings result = boost::get<ResultWithWarnings> (res);

        spline->setParameters (result.x.segment (0, 13));
        spline2->setParameters (result.x.segment (13, 13));
	pythonPlot
	  << (matplotlib
	      << plot_spline (*spline, step)
	      << title ((spline->getName () + title_end).c_str ())
	      << plot_spline (*spline2, step)
	      << title ((spline2->getName () + title_end).c_str ())
	      ) << std::endl;

        std::cout << result << std::endl;
        break;
      }

    case solver_t::SOLVER_NO_SOLUTION:
    case solver_t::SOLVER_ERROR:
      {
        SolverError err = boost::get<SolverError> (res);
        std::cout << "A solution should have been found. Failing..."
		  << std::endl
		  << err.what ()
		  << std::endl;
        Result result = boost::get<Result> (err.lastState ());
        std::cout << result << std::endl;

        pythonPlot
          << (matplotlib
              << plot_spline (*spline, step)
	      << title ((spline->getName () + " initial state").c_str ())
              << plot_spline (*spline2, step)
	      << title ((spline2->getName () + " initial state").c_str ())
             ) << std::endl;

        BOOST_CHECK (false);
      }
    }
}

BOOST_FIXTURE_TEST_SUITE (trajectory, TestSuiteConfiguration)

BOOST_AUTO_TEST_CASE_TEMPLATE (problem_over_splines, spline_t, splinesType_t)
{
  typedef EigenMatrixSparse T;
  typedef Solver<T> solver_t;
  typedef typename spline_t::vector_t param_t;
  typedef GenericFunction<T>::value_type value_type;
  typedef boost::shared_ptr<spline_t> splinePtr_t;
  typedef std::vector<splinePtr_t> splines_t;

  boost::shared_ptr<boost::test_tools::output_test_stream>
    output = retrievePattern ("problem-over-splines");

  std::string spline_name = splineName<spline_t> ();
  boost::filesystem::ofstream pythonPlot
    ("problem-over-splines-" + spline_name + ".py");

  splines_t splines;

  int n = 13;
  param_t params (n);
  params << 40,25,5,0,8,15,0,10,3,-5,15,22,35;

  splinePtr_t spline = boost::make_shared<spline_t>
    (std::make_pair (0,1), 1, params, "B-spline 1", false);
  splines.push_back (spline);

  param_t params2 (n);
  params2 << 10,12,9,8,25,15,24,20,1,5,12,-12,3;
  splinePtr_t spline2 = boost::make_shared<spline_t>
    (std::make_pair (0,1), 1, params2, "B-spline 2", true);
  splines.push_back (spline2);

  // Create the problem
  solver_t::problem_t::intervals_t range;
  std::vector<value_type> range2;
  JerkOverSplinesFactory<spline_t, T>
    jerkFactory (splines, Function::makeInterval (0, 1));
  solver_t::problem_t pb (jerkFactory.getJerk ());

  // Create the factory and add some constraints
  ProblemOverSplinesFactory<T, spline_t> constraints (splines, pb);
  BOOST_CHECK (pb.constraints().size() == 0);

  constraints.updateStartingPoint (0.02);
  range.clear();
  range.push_back (std::make_pair<value_type, value_type> (0, 5));
  range.push_back (std::make_pair<value_type, value_type> (0, 5));

  constraints.addConstraint (0.02, 0, range);
  range.clear ();
  range.push_back (std::make_pair<value_type, value_type> (1, 10));
  range.push_back (std::make_pair<value_type, value_type> (3, 8));
  constraints.addConstraint (0.62, 1, range);

  BOOST_CHECK (constraints.getProblem ().constraints ().size () == 4);
  solver_t::problem_t problem (constraints.getProblem ());

  // Set starting point
  param_t startingPoint (2*n);
  startingPoint << params, params2;
  problem.startingPoint () = startingPoint;

  // Initialize solver
  SolverFactory<solver_t> factory ("ipopt-sparse", problem);
  solver_t& solver = factory ();
  solver.parameters()["ipopt.tol"].value = 1e-3;
  solver.parameters()["ipopt.output_file"].value = std::string("test.log");

  (*output) << solver.problem () << std::endl;

  Matplotlib matplotlib = Matplotlib::make_matplotlib (std::make_pair(3, 2));

  typename solver_t::result_t res = solver.minimum();
  processResult<T, spline_t> (res, spline, spline2, matplotlib, pythonPlot);

  constraints.updateStartingPoint(0.32);
  range2.clear();
  range2.push_back((*spline)(0.32)[0]);
  range2.push_back((*spline2)(0.32)[0]);
  constraints.addConstraint(0.32, 0, range2);
  range2.clear();
  range2.push_back(spline->derivative(0.32, 1)[0]);
  range2.push_back(spline2->derivative(0.32, 1)[0]);
  constraints.addConstraint(0.32, 1, range2);
  range2.clear();
  range2.push_back(spline->derivative(0.32, 2)[0]);
  range2.push_back(spline2->derivative(0.32, 2)[0]);
  constraints.addConstraint(0.32, 2, range2);
  solver_t::problem_t newproblem (constraints.getProblem());
  BOOST_CHECK(newproblem.constraints().size() == 8);
  startingPoint << spline->parameters(), spline2->parameters();
  newproblem.startingPoint() = startingPoint;
  SolverFactory<solver_t> newfactory (TESTSUITE_SOLVER "-sparse", newproblem);
  solver_t& newsolver = newfactory ();

  (*output) << newsolver.problem () << std::endl;

  newsolver.parameters() = solver.parameters ();
  newsolver.parameters()["ipopt.output_file"].value = std::string("test2.log");

  res = newsolver.minimum();
  processResult<T, spline_t> (res, spline, spline2, matplotlib, pythonPlot);

  std::cout << output->str () << std::endl;
  BOOST_CHECK (output->match_pattern ());
}
BOOST_AUTO_TEST_SUITE_END ()
