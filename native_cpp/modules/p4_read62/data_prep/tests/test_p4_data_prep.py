"""Scoped P4 selection and circular wind interpolation tests; no network access."""
import unittest
from native_cpp.modules.p4_read62.data_prep import download_igra as station
from native_cpp.modules.p4_read62.data_prep import parse_igra_to_csv as parse
from native_cpp.modules.p4_read62.data_prep import read as fsl

class P4ScopedTest(unittest.TestCase):
    def test_wind_direction(self):
        for module in (parse, fsl):
            f = module._interp_wind_direction
            for a,b,expected in ((350,10,0),(10,350,0),(10,190,100),(190,10,280)):
                self.assertAlmostEqual(f(a,b,0.5),expected)
            self.assertIsNone(f(None,10,0.5))
        levels = [
            {'pressure_hPa':600,'geopotential height_m':1000,'wind direction_degree':350},
            {'pressure_hPa':400,'geopotential height_m':2000,'wind direction_degree':10}]
        profiles = parse._ensure_target_pressure_level(levels,500)
        target = [r for r in profiles if r['pressure_hPa']==500]
        self.assertEqual(len(target),1)
        self.assertEqual(target[0]['wind direction_degree'],0)

    def test_coverage_strict(self):
        self.assertFalse(station.coverage_passes(584,730))
        self.assertTrue(station.coverage_passes(585,730))
        self.assertFalse(station.coverage_passes(585,732))
        self.assertTrue(station.coverage_passes(586,732))

    def test_inside_and_outside(self):
        stations=[
            dict(id='A',lat=31.94,lon=117.14,first_year=2021,last_year=2024),
            dict(id='B',lat=31.95,lon=117.15,first_year=2022,last_year=2024),
            dict(id='NEAR',lat=31.94,lon=117.22,first_year=2021,last_year=2024),
            dict(id='FAR',lat=31.94,lon=117.35,first_year=2021,last_year=2024),
            dict(id='EXPIRED',lat=31.94,lon=117.14,first_year=2021,last_year=2021)]
        inside,outside=station.candidate_groups(stations,31.94,117.14,0.1,50,50,2022)
        self.assertEqual({x['id'] for x in inside},{'A','B'})
        self.assertEqual([x['id'] for x in outside],['NEAR','FAR'])
        self.assertEqual({x['id'] for x in station.candidate_groups(stations,31.94,117.14,0.1,50,50,2023)[0]},{'A','B'})

if __name__ == '__main__':
    unittest.main()
