# TIREx Tracker Shell API

This package is intended to wrap arbitrary commands in TIRA/TIREx runners so that their resource consumption is monitored. The shell script will test that the tirex-tracker works as expected in an environment and if so, it will wrap an command in the wrapper and if not it will execute it without wrapping.


Usage:

```shell
wget 'https://github.com/tira-io/tirex-tracker/releases/download/0.2.13/measure-0.2.13-linux'
./tirex-tracker.sh your-command-here
```

Unit tests:

```shell
wget 'https://github.com/tira-io/tirex-tracker/releases/download/0.2.13/measure-0.2.13-linux'
pytest test/
```

