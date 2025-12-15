// pybind11
#include <pybind11/operators.h> // To define operator overloading
#include <pybind11/pybind11.h>  // Basic pybind11 functionality
#include <pybind11/stl.h>       // Automatic conversion of vectors

// Local headers
#include "cgal_types.h"
#include "cgal_utils.h"
#include "geometry_operations.h"
#include "flip_partner_map_native.h"

// Pybind11 module definitions
PYBIND11_MODULE(_bindings, m) {
  namespace py = pybind11;
  using namespace cgshop2026;
  m.doc() = "CGAL geometry bindings for CG:SHOP 2026";

  // Exact numbers
  py::class_<Kernel::FT>(m, "FieldNumber",
                         "A container for exact numbers in CGAL.")
      .def(py::init<long>())
      .def(py::init<double>())
      .def(py::init(&str_to_exact))
      .def(py::self / Kernel::FT())
      .def(py::self + Kernel::FT())
      .def(py::self - Kernel::FT())
      .def(py::self * Kernel::FT())
      .def(py::self == Kernel::FT())
      .def(py::self < Kernel::FT())
      .def(py::self > Kernel::FT())
      .def(py::self <= Kernel::FT())
      .def(py::self >= Kernel::FT())
      .def("__float__",
           [](const Kernel::FT &ft) { return CGAL::to_double(ft); })
      .def("__str__",
           [](const Kernel::FT &x) {
             return std::to_string(CGAL::to_double(x));
           })
      .def("exact", &to_rational_string);

  // Points
  py::class_<Point>(m, "Point", "A 2-dimensional point.")
      .def(py::init<long, long>())
      .def(py::init<double, double>())
      .def(py::init<Kernel::FT, Kernel::FT>())
      .def("__add__",
           [](const Point &p1, const Point &p2) {
             // Addition is not defined in CGAL for points
             return Point(p1.x() + p2.x(), p1.y() + p2.y());
           })
      .def("__sub__",
           [](const Point &p1, const Point &p2) {
             return Point(p1.x() - p2.x(), p1.y() - p2.y());
           })
      .def(py::self == Point())
      .def(py::self != Point())
      .def("x", [](const Point &p) { return p.x(); })
      .def("y", [](const Point &p) { return p.y(); })
      .def("__len__", [](const Point &self) { return 2; })
      .def("__getitem__",
           [](const Point &self, int i) {
             if (i == 0) {
               return self.x();
             } else if (i == 1) {
               return self.y();
             }
             throw std::out_of_range("Only 0=x and 1=y.");
           })
      .def("__str__", &point_to_string);

  // Segments
  py::class_<Segment2>(m, "Segment", "A 2-dimensional segment.")
      .def(py::init<Point, Point>())
      .def("source", &Segment2::source)
      .def("target", &Segment2::target)
      .def("__str__", [](const Segment2 &self) {
        return fmt::format("[{}, {}]", point_to_string(self.source()),
                           point_to_string(self.target()));
      });

  // Triangulation check
  m.def("is_triangulation", &is_triangulation,
        "Check if a set of edges forms a triangulation of the given points.",
        py::arg("points"), py::arg("edges"), py::arg("verbose") = false);

  // Compute triangles
  m.def("compute_triangles", &compute_triangles,
        "Compute all triangles formed by the given points and edges.");

  // Segment crossing test
  m.def("do_cross", &do_cross, "Check if two segments cross each other.");

  m.def("violates_local_delaunay", &violates_local_delaunay,
        "Return true if edge ab fails the empty circumcircle test with respect to "
        "opposite points c and d.",
        py::arg("a"), py::arg("b"), py::arg("c"), py::arg("d"));

  m.def("sample_parallel_batch", &sample_parallel_batch,
        "Sample a batch of compatible flips for simulated annealing.",
        py::arg("points"), py::arg("edges"), py::arg("candidates"),
        py::arg("offenders"), py::arg("random_pick_prob"),
        py::arg("seed"));

  m.def("compute_delaunay_batches", &compute_delaunay_batches,
        "Compute batches of flips that deterministically finish to Delaunay.",
        py::arg("points"), py::arg("edges"));

  py::class_<FlipPartnerMapNative>(m, "FlipPartnerMapNative",
                                   "Native flip-partner map implementation.")
      .def(py::init<const std::vector<Point> &,
                    const std::vector<std::tuple<int, int>> &>())
      .def("compute_triangles", &FlipPartnerMapNative::compute_triangles)
      .def("is_flippable", &FlipPartnerMapNative::is_flippable)
      .def("flippable_edges", &FlipPartnerMapNative::flippable_edges)
      .def("get_flip_partner", &FlipPartnerMapNative::get_flip_partner)
      .def("conflicting_flips", &FlipPartnerMapNative::conflicting_flips)
      .def("flip", &FlipPartnerMapNative::flip)
      .def("edges", &FlipPartnerMapNative::edges)
      .def("points", &FlipPartnerMapNative::points)
      .def("deep_copy", &FlipPartnerMapNative::deep_copy);
}
