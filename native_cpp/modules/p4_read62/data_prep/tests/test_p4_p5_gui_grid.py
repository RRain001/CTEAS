"""P6G G5: GUI legacy and native station selectors share the published UTM grid."""
from __future__ import annotations

import ast
import sys
import tempfile
import unittest
from datetime import datetime
from pathlib import Path
from unittest.mock import patch

from engine.preprocess.read62_upperair_data import download_igra as legacy_p4
from engine.preprocess.read62_upperair_data import run_upperair_pipeline as p4_pipeline
from engine.run_met.common import GridDef, lon_lat_to_utm_km
from engine.run_met.station_grid import lon_lat_in_utm_grid
from native_cpp.modules.p4_read62.data_prep import download_igra as native_p4
from native_cpp.modules.p5_smerge.data_prep import download_isd_by_grid as native_p5


class SharedLegacyGridTest(unittest.TestCase):
    @staticmethod
    def grid() -> GridDef:
        x, y = lon_lat_to_utm_km(117.14, 31.94, 50, True)
        return GridDef("UTM", 50, "N", "WGS-84", x-2.5, y-2.5,
                       50, 50, 0.1, 1, [0.0, 20.0])

    def test_legacy_native_p4_and_p5_agree_by_year(self):
        grid = self.grid()
        rows = [
            dict(id="A", lat=31.94, lon=117.14, first_year=2021, last_year=2024),
            dict(id="B", lat=31.95, lon=117.15, first_year=2023, last_year=2024),
            dict(id="FAR", lat=31.94, lon=117.33, first_year=2021, last_year=2024),
        ]
        for year, expected in ((2022, {"A"}), (2023, {"A", "B"})):
            exists, legacy = legacy_p4.choose_station(rows, 31.94, 117.14, grid, year)
            observed, outside = native_p4.candidate_groups(rows, 31.94, 117.14, grid, year)
            self.assertTrue(exists)
            self.assertEqual({s["id"] for s in legacy}, expected)
            self.assertEqual({s["id"] for s in observed}, expected)
            self.assertEqual([s["id"] for s in outside], ["FAR"])
            for station in rows:
                self.assertEqual(
                    lon_lat_in_utm_grid(station["lon"], station["lat"], grid),
                    native_p5.station_in_utm_grid(
                        type("S", (), {"lon": station["lon"], "lat": station["lat"]})(), grid))

    def test_legacy_no_lat_lon_bbox_and_shifted_origin(self):
        paths = [
            Path("engine/preprocess/read62_upperair_data/download_igra.py"),
            Path("engine/preprocess/smerge_surf_data/download_isd_by_grid.py"),
            Path("native_cpp/modules/p4_read62/data_prep/download_igra.py"),
            Path("native_cpp/modules/p5_smerge/data_prep/download_isd_by_grid.py"),
        ]
        for path in paths:
            funcs = {n.name for n in ast.walk(ast.parse(path.read_text()))
                     if isinstance(n, ast.FunctionDef)}
            self.assertNotIn("build_bbox", funcs)
            self.assertNotIn("signed_offsets_km", funcs)
            self.assertNotIn("inside_bbox", funcs)
        grid = self.grid()
        s = dict(id="A", lat=31.94, lon=117.14, first_year=2021, last_year=2024)
        with self.subTest("normal"):
            self.assertTrue(legacy_p4.choose_station([s], 31.94, 117.14, grid, 2022)[0])
        with self.subTest("shifted"):
            from dataclasses import replace
            shifted = replace(grid, xrefkm=grid.xrefkm+20)
            self.assertFalse(legacy_p4.choose_station([s], 31.94, 117.14, shifted, 2022)[0])

    def test_gui_pipeline_auto_reselects_each_year(self):
        dispatched = []
        cfg = {"preprocess": {"upperair": {"auto_select_all": True}}}
        with patch.object(p4_pipeline, "load_case_config", return_value=cfg), \
             patch.object(p4_pipeline, "parse_research_time",
                          return_value=(datetime(2022, 12, 31), datetime(2023, 1, 1))), \
             patch.object(p4_pipeline, "run_step",
                          side_effect=lambda title, cmd: dispatched.append((title, cmd))), \
             patch.object(p4_pipeline, "read_selected_station_id",
                          side_effect=AssertionError("must not pin prior year")), \
             patch.object(sys, "argv", ["p4_pipeline", "--pstop", "500"]):
            self.assertEqual(p4_pipeline.main(), 0)
        downloads = [cmd for title, cmd in dispatched if "下载IGRA" in title]
        self.assertEqual(len(downloads), 2)
        self.assertEqual([cmd[cmd.index("--year")+1] for cmd in downloads],
                         ["2022", "2023"])
        self.assertTrue(all("--auto-select-all" in cmd and "--station-id" not in cmd
                            for cmd in downloads))


if __name__ == "__main__":
    unittest.main()
