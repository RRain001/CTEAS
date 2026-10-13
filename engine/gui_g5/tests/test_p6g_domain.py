"""P6G G6 station-envelope planning: explicit inputs, no implicit native physics."""
import json
import math
import tempfile
import unittest
from pathlib import Path

from engine.gui_g5.case_setup import initial_config, write_case_grid
from engine.gui_g5.p6_domain import Station, fit_domain, read_stations
from engine.run_met.common import lon_lat_to_utm_km
from engine.run_met.station_grid import load_strict_utm_grid,lon_lat_in_utm_grid


class G6DomainTests(unittest.TestCase):
    def setUp(self):
        self.config=initial_config("G6_TEST")
        g=self.config["preprocess"]["grid"]
        g["center"]={"longitude":117.14,"latitude":31.94}
        g["domain"]={"length_km":5.0,"width_km":5.0}
        g["resolution_km"]=0.1
        self.stations=[
            Station("S1","surface",117.14,31.94),
            Station("S2","surface",117.32,31.95),
            Station("U1","upper",117.12,31.92),
            Station("U2","upper",117.38,31.99),
        ]

    def test_expansion_and_coarsening_preserve_all_four_stations(self):
        plan=fit_domain(self.config,self.stations,margin_km=3.0,
                        max_horizontal_cells=2500)
        target=plan["recommended"]
        self.assertGreater(target["length_km"],5)
        self.assertGreater(target["width_km"],5)
        self.assertGreater(target["spacing_km"],0.1)
        self.assertLessEqual(target["horizontal_cells"],2500)
        self.assertEqual(plan["station_counts"],{"surface":2,"upper":2})
        self.assertFalse(plan["native_p6d_physics_accepted"])
        self.assertEqual(self.config["preprocess"]["grid"]["resolution_km"],0.1)
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            write_case_grid(root,plan["proposed_config"])
            grid=load_strict_utm_grid(root/"grid"/"grid.cmn")
            self.assertEqual((grid.nx,grid.ny),
                             (target["nx"],target["ny"]))
            for s in self.stations:
                self.assertTrue(lon_lat_in_utm_grid(s.lon,s.lat,grid),s.id)

    def test_short_envelope_no_forced_coarsening(self):
        near=[Station("S","surface",117.14,31.94),
              Station("U","upper",117.1402,31.9401)]
        p=fit_domain(self.config,near,margin_km=0.0,
                     max_horizontal_cells=2500)
        self.assertAlmostEqual(p["recommended"]["spacing_km"],0.1)
        self.assertEqual(p["recommended"]["nx"],50)

    def test_reject_cross_hemisphere_and_distant_utm_zone(self):
        bad=[Station("x","upper",150.0,31.94)]
        with self.assertRaisesRegex(ValueError,"meridian"):
            fit_domain(self.config,bad)
        with self.assertRaisesRegex(ValueError,"hemisphere"):
            fit_domain(self.config,[Station("y","upper",117.14,-10.0)])

    def test_reject_infeasible_budget_and_invalid_station_manifest(self):
        with self.assertRaisesRegex(ValueError,"Cannot fit"):
            fit_domain(self.config,self.stations,max_horizontal_cells=1)
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/"stations.json"
            p.write_text(json.dumps([
                {"id":"U1","kind":"upper","lon":117.14,"lat":31.94},
                {"id":"U1","kind":"upper","lon":117.15,"lat":31.95}]))
            with self.assertRaisesRegex(ValueError,"Duplicate"):
                read_stations(p)
            p.write_text(json.dumps([
                {"id":"S1","kind":"surface","lon":117.14,"lat":31.94},
                {"id":"U1","kind":"upper","lon":117.16,"lat":31.92}]))
            self.assertEqual(len(read_stations(p)),2)

    def test_immutable_planner_and_cost_separation(self):
        source=initial_config("IMMUTABLE")
        old=source["preprocess"]["grid"]["domain"]["length_km"]
        plan=fit_domain(source,[Station("S","surface",117.14,31.94)],
                        max_horizontal_cells=5000)
        self.assertEqual(source["preprocess"]["grid"]["domain"]["length_km"],old)
        self.assertEqual(plan["existing"]["horizontal_cells"],2500)
        self.assertGreater(plan["recommended"]["3d_cells"],0)


if __name__=="__main__":
    unittest.main()
