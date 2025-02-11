build-pypi-package: run-tests
	python3 -m build --sdist .
	python3 -m build --wheel .
	twine upload dist/tira_measure-0.0.6.dev36+g3eb1e78.d20250211-py3-none-any.whl dist/tira_measure-0.0.6.dev36+g3eb1e78.d20250211.tar.gz

run-tests:
	PYTHONPATH=. pytest

