def is_pyterrier_installed() -> bool:
    """Check if PyTerrier is installed."""
    try:
        import pyterrier  # noqa: F401

        return True
    except ImportError:
        return False
