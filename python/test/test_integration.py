import unittest
import tempfile
import shutil
from pathlib import Path
from subprocess import check_output
import os
import yaml
from zipfile import ZipFile

class TestIntegration(unittest.TestCase):

    def test_python_script(self):
        with tempfile.TemporaryDirectory() as working_dir:
            working_dir = Path(working_dir).resolve().absolute()
            src_file = Path(__file__).parent.parent / 'examples' / 'example.py'
            shutil.copy(src_file, working_dir / 'example.py')

            env = os.environ.copy()
            if 'PYTHONPATH' in env:
                env['PYTHONPATH'] += ':' + str(Path(__file__).parent.parent.resolve().absolute())

            self.assertFalse((working_dir / 'irmetadata.yml').exists())
            
            check_output(['python3', 'example.py'], cwd=str(working_dir), env=env)
            
            self.assertTrue((working_dir / 'irmetadata.yml').exists())
            with open(working_dir / 'irmetadata.yml', 'r') as f:
                truncated_content = '\n'.join(f.readlines()[1:-1])
                yaml_content = yaml.safe_load(truncated_content)

            self.assertIn('implementation', yaml_content)
            self.assertIn('script', yaml_content['implementation'])
            self.assertIn('path', yaml_content['implementation']['script'])
            self.assertIn('example.py', yaml_content['implementation']['script']['path'])
            self.assertIn('code', yaml_content['implementation'])
            self.assertIn('archive', yaml_content['implementation']['code'])
            self.assertEqual('code.zip', yaml_content['implementation']['code']['archive'])

            zipObj = ZipFile(working_dir / 'code.zip')
            files_in_zip = [i.filename for i in zipObj.infolist()]

            self.assertEqual(['example.py'], files_in_zip)

    def test_jupyter_notebook(self):
        with tempfile.TemporaryDirectory() as working_dir:
            working_dir = Path(working_dir).resolve().absolute()
            src_file = Path(__file__).parent.parent / 'examples' / 'example-notebook.ipynb'
            shutil.copy(src_file, working_dir / 'example-notebook.ipynb')

            env = os.environ.copy()
            if 'PYTHONPATH' in env:
                env['PYTHONPATH'] += ':' + str(Path(__file__).parent.parent.resolve().absolute())

            self.assertFalse((working_dir / 'irmetadata.yml').exists())
            
            check_output(['runnb', '--allow-not-trusted', 'example-notebook.ipynb'], cwd=str(working_dir), env=env)
            
            self.assertTrue((working_dir / 'irmetadata.yml').exists())
            with open(working_dir / 'irmetadata.yml', 'r') as f:
                truncated_content = '\n'.join(f.readlines()[1:-1])
                yaml_content = yaml.safe_load(truncated_content)

            self.assertIn('implementation', yaml_content)
            self.assertIn('script', yaml_content['implementation'])
            self.assertIn('path', yaml_content['implementation']['script'])
            self.assertIn('script.py', yaml_content['implementation']['script']['path'])
            self.assertIn('code', yaml_content['implementation'])
            self.assertIn('archive', yaml_content['implementation']['code'])
            self.assertEqual('code.zip', yaml_content['implementation']['code']['archive'])

            zipObj = ZipFile(working_dir / 'code.zip')
            files_in_zip = [i.filename for i in zipObj.infolist()]

            self.assertEqual(['script.py', 'notebook.ipynb'], files_in_zip)

