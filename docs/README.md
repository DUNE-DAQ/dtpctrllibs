# dtpctrllibs

This package contains DAQ modules for controlling DUNE Trigger Primitive generation firmware.

## DTPController

This is the module which controls TP firmware. It is a very simple module, with no I/O queues.  It responds to several RC commands :
   * conf : this command will establish the connection to the firmware (ipbus over flx), and configure it
   * reset : this command just resets firmware
   * start : currently does nothing
   * stop : currently does nothing

### Configuration

Here we will put info about the configuration data

### Monitoring

Here we will put info about the monitored data


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
