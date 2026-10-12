import importlib.util
import pathlib
import sys
import tempfile
import unittest

SCRIPTS=pathlib.Path(__file__).resolve().parents[1]/'scripts'
sys.path.insert(0,str(SCRIPTS))
from resource_limits import detect
from gate_worker import admit


class ResourceControlTests(unittest.TestCase):
    def fixture(self, root, quota='max 100000', cpuset='0-7', memory='max'):
        root.mkdir(parents=True,exist_ok=True)
        for name,value in {'cgroup.controllers':'cpu memory cpuset','cpu.max':quota,
                           'cpuset.cpus.effective':cpuset,'memory.max':memory,'memory.high':'max'}.items():
            (root/name).write_text(value)
        return root

    def test_quotas_2_4_6_and_request_clamping(self):
        with tempfile.TemporaryDirectory() as d:
            root=pathlib.Path(d)/'cg'
            for cpu in (2,4,6):
                self.fixture(root,f'{cpu*100000} 100000')
                self.assertEqual(detect('24',root,set(range(8)))['build_jobs'],cpu)
                self.assertEqual(detect('1',root,set(range(8)))['build_jobs'],1)

    def test_parent_limit_and_cpuset_affinity(self):
        with tempfile.TemporaryDirectory() as d:
            root=self.fixture(pathlib.Path(d)/'cg','400000 100000')
            leaf=self.fixture(root/'job','max 100000','1-2')
            self.assertEqual(detect(None,leaf,set(range(8)))['build_jobs'],2)
            self.assertEqual(detect(None,leaf,{2,3})['build_jobs'],1)

    def test_fractional_quota_and_memory_budget(self):
        with tempfile.TemporaryDirectory() as d:
            root=self.fixture(pathlib.Path(d)/'cg','250000 100000')
            self.assertEqual(detect(None,root,set(range(8)))['build_jobs'],2)
            self.fixture(root,'600000 100000',memory=str(1024**3))
            self.assertEqual(detect(None,root,set(range(8)))['build_jobs'],1)
            self.assertEqual(detect(None,root,set(range(8)))['memory_budget_bytes'],1024**3)

    def test_unlimited_safe_cap_and_invalid_request(self):
        with tempfile.TemporaryDirectory() as d:
            root=self.fixture(pathlib.Path(d)/'cg',cpuset='0-111')
            self.assertEqual(detect(None,root,set(range(112)))['build_jobs'],8)
            self.assertEqual(detect('24',root,set(range(112)))['build_jobs'],24)
            for value in ('0','-1','bad'):
                with self.assertRaises(ValueError):detect(value,root,set(range(8)))

    def test_pool_admission_and_same_track(self):
        active=[dict(track='p6e',cpus=2,memory_gib=4)]
        self.assertTrue(admit(active,dict(track='shared',cpus=4,memory_gib=10)))
        self.assertFalse(admit(active,dict(track='p6e',cpus=2,memory_gib=4)))
        self.assertFalse(admit(active,dict(track='p7',cpus=6,memory_gib=15)))
        self.assertFalse(admit(active,dict(track='p7',cpus=4,memory_gib=12)))


if __name__=='__main__':unittest.main()
