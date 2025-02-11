build-pypi-package: run-tests
	rm -Rf dist
	python3 -m build --sdist .
	python3 -m build --wheel .
	twine upload dist/tira_measure-*.whl dist/tira_measure-*.tar.gz

run-tests:
	PYTHONPATH=. pytest

