# dtpctrllibs

This package contains DAQ modules for controlling DUNE Trigger Primitive generation firmware.

## Running tests

A quick recipe for running tests.

1. Setup two shells based on your working dunedaq environment.
2. In shell 1 :
```
daq_application --name dtp_app --commandFacility rest://localhost:12345
```
3. In shell 2 :
```
python -m dtpctrllibs.dtp_app_confgen dtp_app.json
send-recv-restcmd.py --interactive --file dtp_app.json
```

At the terminal that now appears, run 'init', 'conf' and 'reset' commands.