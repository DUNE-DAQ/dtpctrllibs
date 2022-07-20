# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('cmdlib/cmd.jsonnet')
moo.otypes.load_types('rcif/cmd.jsonnet')
moo.otypes.load_types('appfwk/cmd.jsonnet')
moo.otypes.load_types('appfwk/app.jsonnet')
moo.otypes.load_types('readoutlibs/readoutconfig.jsonnet')
moo.otypes.load_types('readoutlibs/recorderconfig.jsonnet')
moo.otypes.load_types('flxlibs/felixcardreader.jsonnet')
moo.otypes.load_types('flxlibs/felixcardcontroller.jsonnet')
moo.otypes.load_types('dtpctrllibs/dtpcontroller.jsonnet')

# Import new types
import dunedaq.cmdlib.cmd as basecmd # AddressedCmd, 
import dunedaq.rcif.cmd as rccmd # AddressedCmd, 
import dunedaq.appfwk.app as app # AddressedCmd, 
import dunedaq.appfwk.cmd as cmd # AddressedCmd, 
import dunedaq.readoutlibs.readoutconfig as rconf 
import dunedaq.readoutlibs.recorderconfig as bfs
import dunedaq.flxlibs.felixcardreader as flxcr
import dunedaq.flxlibs.felixcardcontroller as flxcc
import dunedaq.iomanager.connection as conn
#import dunedaq.networkmanager.nwmgr as nwmgr
import dunedaq.dtpctrllibs.dtpcontroller as dtpcontroller

from appfwk.utils import mcmd, mrccmd, mspec

import json
import math
import re
# Time to wait on pop()
QUEUE_POP_WAIT_MS=100;
# local clock speed Hz
CLOCK_SPEED_HZ = 50000000;

def generate(
        FRONTEND_TYPE='wib',
        NUMBER_OF_DATA_PRODUCERS=1,          
        NUMBER_OF_TP_PRODUCERS=1,      
        FELIX_ELINK_MASK="0:0",    
        DATA_RATE_SLOWDOWN_FACTOR = 1,
        EMULATOR_MODE = False,
        ENABLE_SOFTWARE_TPG=False,
        RUN_NUMBER = 333,
        SUPERCHUNK_FACTOR=12,
        EMU_FANOUT=False,
        DATA_FILE="./frames.bin",
        TPG_CHANNEL_MAP="ProtoDUNESP1ChannelMap",
        CONNECTIONS_FILE="${DTPCONTROLS_SHARE}/config/dtp_connections.xml",
        DATA_SOURCE="int",
        WIBULATOR_DATA="./dtp-patterns/FixedHits/A/FixedHits_A_wib_axis_32b.txt",
        UHAL_LOG_LEVEL="debug",
        OUTPUT_PATH="."
    ):

    n_tp_link_0 = 1
    n_tp_link_1 = 1
    # Define modules and queues
    queue_specs = [
        conn.ConnectionId(uid="data_fragments_q", service_type="kQueue", data_type="", uri=f"queue://FollyMPMC:100"),
        conn.ConnectionId(uid="errored_chunks_q", service_type="kQueue", data_type="", uri=f"queue://FollyMPMC:100"),
        conn.ConnectionId(uid="errored_frames_q", service_type="kQueue", data_type="", uri=f"queue://FollyMPMC:10000"),
        conn.ConnectionId(uid=f"raw_tp_link_0", service_type="kQueue", data_type="raw_tp", uri=f"queue://FollySPSC:100000"),
        conn.ConnectionId(uid=f"raw_tp_link_1", service_type="kQueue", data_type="raw_tp", uri=f"queue://FollySPSC:100000"),
        conn.ConnectionId(uid="data_requests_0", service_type="kQueue", data_type="", uri=f"queue://FollySPSC:1000"),
        conn.ConnectionId(uid="data_requests_1", service_type="kQueue", data_type="", uri=f"queue://FollySPSC:1000"),
        conn.ConnectionId(uid="time_sync_q", topics=["Timesync"], service_type="kNetSender", data_type="", uri=f"tcp://127.0.0.1:6000") 
    ]

    # Only needed to reproduce the same order as when using jsonnet
    #queue_specs = app.QueueSpecs(sorted(queue_bare_specs, key=lambda x: x.inst))

    #! omit DataRecorder for TP's for now
    mod_specs = [
        mspec(f"datahandler_0", "DataLinkHandler", [
            conn.ConnectionRef(name="raw_input", uid=f"raw_tp_link_0", dir="kInput"),
            conn.ConnectionRef(name="timesync_output", uid="time_sync_q", dir="kOutput"),
            conn.ConnectionRef(name="request_input", uid="data_requests_0", dir="kInput"),
            conn.ConnectionRef(name="fragment_queue", uid="data_fragments_q", dir="kOutput"),
            conn.ConnectionRef(name="errored_frames", uid="errored_frames_q", dir="kOutput"),
        ])
    ] + [
        mspec(f"datahandler_1", "DataLinkHandler", [
            conn.ConnectionRef(name="raw_input", uid=f"raw_tp_link_1", dir="kInput"),
            conn.ConnectionRef(name="timesync_output", uid="time_sync_q", dir="kOutput"),
            conn.ConnectionRef(name="request_input", uid="data_requests_1", dir="kInput"),
            conn.ConnectionRef(name="fragment_queue", uid="data_fragments_q", dir="kOutput"),
            conn.ConnectionRef(name="errored_frames", uid="errored_frames_q", dir="kOutput"),
        ])
    ]

    mod_specs.append(mspec("flxcard_0", "FelixCardReader", [
        conn.ConnectionRef(name=f"output_5", uid=f"raw_tp_link_0", dir="kOutput")
    ] + [
        conn.ConnectionRef(name="errored_chunks", uid="errored_chunks_q", dir="kOutput")
    ]))

    mod_specs.append(mspec("flxcard_1", "FelixCardReader", [
        conn.ConnectionRef(name=f"output_11", uid=f"raw_tp_link_1", dir="kOutput")
    ] + [
        conn.ConnectionRef(name="errored_chunks", uid="errored_chunks_q", dir="kOutput")
    ]))
        
    mod_specs.append(mspec("flxcardctrl_0", "FelixCardController", []))
    mod_specs.append(mspec("dtpctrl_0", "DTPController", []))
    mod_specs.append(mspec("dtpctrl_1", "DTPController", []))
    init_specs = app.Init(connections=queue_specs, modules=mod_specs)
    
    jstr = json.dumps(init_specs.pod(), indent=4, sort_keys=True)
    print(jstr)
    
    initcmd = rccmd.RCCommand(
        id=basecmd.CmdId("init"),
        entry_state="NONE",
        exit_state="INITIAL",
        data=init_specs
    )
    
    CARDID = 0
    
    lb_size = 3*CLOCK_SPEED_HZ/(25*12*DATA_RATE_SLOWDOWN_FACTOR)
    lb_remainder = lb_size % 4096
    lb_size -= lb_remainder # ensure latency buffer size is always a multiple of 4096, so should be 4k aligned

    confcmd = mrccmd("conf", "INITIAL", "CONFIGURED", [
        ("flxcard_0",flxcr.Conf(card_id=CARDID,
                                logical_unit=0,
                                dma_id=0,
                                chunk_trailer_size= 32,
                                dma_block_size_kb= 4,
                                dma_memory_size_gb= 4,
                                numa_id=0,
                                links_enabled=[ 5 ])),
        ("flxcard_1",flxcr.Conf(card_id=CARDID,
                                logical_unit=1,
                                dma_id=0,
                                chunk_trailer_size= 32,
                                dma_block_size_kb= 4,
                                dma_memory_size_gb= 4,
                                numa_id=0,
                                links_enabled=[ 5 ])),
        ("flxcardctrl_0",flxcc.Conf(
            card_id=CARDID,
            logical_units=[flxcc.LogicalUnit(
                log_unit_id=0,
                emu_fanout=EMU_FANOUT,
                links=[flxcc.Link(
                    link_id=5, 
                    enabled=True, 
                    dma_desc=0, 
                    superchunk_factor=SUPERCHUNK_FACTOR)]),
                           flxcc.LogicalUnit(
                               log_unit_id=1,
                               emu_fanout=EMU_FANOUT,
                               links=[flxcc.Link(
                                   link_id=5, 
                                   enabled=True, 
                                   dma_desc=0, 
                                   superchunk_factor=SUPERCHUNK_FACTOR)])])),
        ("dtpctrl_0", dtpcontroller.ConfParams(
            connections_file=CONNECTIONS_FILE,
            device="flx-0-p2-hf",
            source=DATA_SOURCE,
            pattern=WIBULATOR_DATA,
            uhal_log_level=UHAL_LOG_LEVEL,
            threshold = 20,
            masks = [
                0xfffffffe,
                0xffffffff,
                0xffffffff,
                0xffffffff,
                0xfffffffe,
                0xffffffff,
                0xffffffff,
                0xffffffff,
                0xfffffffe,
                0xffffffff,
                0xffffffff,
                0xffffffff,
                0xfffffffe,
                0xffffffff,
                0xffffffff,
                0xffffffff,
                0xfffffffe,
                0xffffffff,
                0xffffffff,
                0xffffffff ])),
        ("dtpctrl_1", dtpcontroller.ConfParams(
            connections_file=CONNECTIONS_FILE,
            device="flx-1-p2-hf",
            source=DATA_SOURCE,
            pattern=WIBULATOR_DATA,
            uhal_log_level=UHAL_LOG_LEVEL,
            threshold = 20,
            masks = [
                0xfffffffe,
                0xffffffff,
                0xffffffff,
                0xffffffff,
                0xfffffffe,
                0xffffffff,
                0xffffffff,
                0xffffffff,
                0xfffffffe,
                0xffffffff,
                0xffffffff,
                0xffffffff,
                0xfffffffe,
                0xffffffff,
                0xffffffff,
                0xffffffff,
                0xfffffffe,
                0xffffffff,
                0xffffffff,
                0xffffffff ]))
    ] + [
        (f"datahandler_0", rconf.Conf(
            readoutmodelconf= rconf.ReadoutModelConf(
                source_queue_timeout_ms= QUEUE_POP_WAIT_MS,
                fake_trigger_flag=1,
                timesync_connection_name="timesync",
                region_id = 0,
                element_id = 1,
            ),
            latencybufferconf= rconf.LatencyBufferConf(
                latency_buffer_alignment_size = 4096,
                latency_buffer_size = lb_size,
                region_id = 0,
                element_id = 0,
            ),
            rawdataprocessorconf= rconf.RawDataProcessorConf(
                region_id = 0,
                element_id = 0,
                enable_software_tpg = False,
                enable_firmware_tpg = True,
                emulator_mode = EMULATOR_MODE,
                channel_map_name = TPG_CHANNEL_MAP,
            ),
            requesthandlerconf= rconf.RequestHandlerConf(
                latency_buffer_size = lb_size,
                pop_limit_pct = 0.8,
                pop_size_pct = 0.1,
                region_id = 0,
                element_id = 0,
                output_file = f"raw_output_0.out",
                stream_buffer_size = 8388608,
                enable_raw_recording = True
            )
        ))
    ] + [
        (f"datahandler_1", rconf.Conf(
            readoutmodelconf= rconf.ReadoutModelConf(
                source_queue_timeout_ms= QUEUE_POP_WAIT_MS,
                fake_trigger_flag=1,
                timesync_connection_name="timesync",
                region_id = 0,
                element_id = 1,
            ),
            latencybufferconf= rconf.LatencyBufferConf(
                latency_buffer_alignment_size = 4096,
                latency_buffer_size = lb_size,
                region_id = 0,
                element_id = 1,
            ),
            rawdataprocessorconf= rconf.RawDataProcessorConf(
                region_id = 0,
                element_id = 1,
                enable_software_tpg = False,
                enable_firmware_tpg = True,
                emulator_mode = EMULATOR_MODE,
                channel_map_name = TPG_CHANNEL_MAP,
            ),
            requesthandlerconf= rconf.RequestHandlerConf(
                latency_buffer_size = lb_size,
                pop_limit_pct = 0.8,
                pop_size_pct = 0.1,
                region_id = 0,
                element_id = 1,
                output_file = f"raw_output_1.out",
                stream_buffer_size = 8388608,
                enable_raw_recording = True
            )
        ))
    ])

    jstr = json.dumps(confcmd.pod(), indent=4, sort_keys=True)
    print(jstr)
    
    startpars = rccmd.StartParams(run=RUN_NUMBER)
    startcmd = mrccmd("start", "CONFIGURED", "RUNNING", [
        ("datahandler_.*", startpars),
        ("flxcard_.*", startpars),
        ("dtpctrl_.*", None),
        ("data_recorder_.*", startpars),
        ("fragment_consumer", startpars)
    ])
    
    jstr = json.dumps(startcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStart\n\n", jstr)
    
    
    stopcmd = mrccmd("stop", "RUNNING", "CONFIGURED", [
        ("flxcard_.*", None),
        ("dtpctrl_.*", None),
        ("datahandler_.*", None),
        ("data_recorder_.*", None),
        ("fragment_consumer", None)
    ])
    
    jstr = json.dumps(stopcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStop\n\n", jstr)

    scrapcmd = mrccmd("scrap", "CONFIGURED", "INITIAL", [
        ("", None)
    ])

    jstr = json.dumps(scrapcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nScrap\n\n", jstr)

    # Create a list of commands
    cmd_seq = [initcmd, confcmd, startcmd, stopcmd, scrapcmd]

    record_cmd = mrccmd("record", "RUNNING", "RUNNING", [
        ("datahandler_.*", rconf.RecordingParams(
            duration=10
        ))
    ])
    
    jstr = json.dumps(record_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nRecord\n\n", jstr)

    cmd_seq.append(record_cmd)

    get_reg_cmd = mrccmd("getregister", "RUNNING", "RUNNING", [
        ("flxcardctrl_.*", flxcc.GetRegisters(
            card_id=0,
            log_unit_id=0,
            reg_names=(
                "REG_MAP_VERSION",
            )
        ))
    ])

    jstr = json.dumps(get_reg_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nGet Register\n\n", jstr)

    cmd_seq.append(get_reg_cmd)

    set_reg_cmd = mrccmd("setregister", "RUNNING", "RUNNING", [
        ("flxcardctrl_.*", flxcc.SetRegisters(
            card_id=0,
            log_unit_id=0,
            reg_val_pairs=(
                flxcc.RegValPair(reg_name="REG_MAP_VERSION", reg_val=0),
            )
        ))
    ])

    jstr = json.dumps(set_reg_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nSet Register\n\n", jstr)

    cmd_seq.append(set_reg_cmd)

    get_bf_cmd = mrccmd("getbitfield", "RUNNING", "RUNNING", [
        ("flxcardctrl_.*", flxcc.GetBFs(
            card_id=0,
            log_unit_id=0,
            bf_names=(
                "REG_MAP_VERSION",
            )
        ))
    ])

    jstr = json.dumps(get_bf_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nGet Bitfield\n\n", jstr)

    cmd_seq.append(get_bf_cmd)

    set_bf_cmd = mrccmd("setbitfield", "RUNNING", "RUNNING", [
        ("flxcardcontrol_.*", flxcc.SetBFs(
            card_id=0,
            log_unit_id=0,
            bf_val_pairs=(
                flxcc.RegValPair(reg_name="REG_MAP_VERSION", reg_val=0),
            )
        ))
    ])

    jstr = json.dumps(set_bf_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nSet Bitfield\n\n", jstr)

    cmd_seq.append(set_bf_cmd)

    # Print them as json (to be improved/moved out)
    jstr = json.dumps([c.pod() for c in cmd_seq], indent=4, sort_keys=True)
    return jstr
        
if __name__ == '__main__':
    # Add -h as default help option
    CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

    import click

    @click.command(context_settings=CONTEXT_SETTINGS)
    @click.option('-f', '--frontend-type', type=click.Choice(['wib', 'wib2', 'pds_queue', 'pds_list'], case_sensitive=True), default='wib')
    @click.option('-n', '--number-of-data-producers', default=2)
    @click.option('-t', '--number-of-tp-producers', default=0)
    @click.option('-m', '--felix-elink-mask', default="0:0")
    @click.option('-s', '--data-rate-slowdown-factor', default=10)
    @click.option('-e', '--emulator_mode', is_flag=True)   
    @click.option('-g', '--enable-software-tpg', is_flag=True)
    @click.option('-r', '--run-number', default=333)
    @click.option('-S', '--superchunk-factor', default=12)
    @click.option('-E', '--emu-fanout', is_flag=True)
    @click.option('-d', '--data-file', type=click.Path(), default='./frames.bin')
    @click.option('-c', '--tpg-channel-map', type=click.Choice(["VDColdboxChannelMap", "ProtoDUNESP1ChannelMap"]), default="ProtoDUNESP1ChannelMap")
    @click.option('-c', '--connections-file', default="${DTPCONTROLS_SHARE}/config/dtp_connections.xml")
    @click.option('-u', '--uhal-log-level', default="notice")
    @click.option('-S', '--source-data', default="int")
    @click.option('-w', '--wibulator-data', default="pattern.txt")
    @click.option('-o', '--output-path', type=click.Path(), default='.')
    @click.argument('json_file', type=click.Path(), default='flx_readout.json')
    def cli(frontend_type, number_of_data_producers, number_of_tp_producers, felix_elink_mask, data_rate_slowdown_factor, emulator_mode, enable_software_tpg, run_number, superchunk_factor, emu_fanout, data_file, tpg_channel_map, connections_file, uhal_log_level, source_data, wibulator_data, output_path, json_file):

        """
          JSON_FILE: Input raw data file.
          JSON_FILE: Output json configuration file.
        """

        with open(json_file, 'w') as f:
            f.write(generate(
                    FRONTEND_TYPE = frontend_type,
                    NUMBER_OF_DATA_PRODUCERS = number_of_data_producers,
                    NUMBER_OF_TP_PRODUCERS = number_of_tp_producers,
                    FELIX_ELINK_MASK = felix_elink_mask,
                    DATA_RATE_SLOWDOWN_FACTOR = data_rate_slowdown_factor,
                    EMULATOR_MODE = emulator_mode,
                    ENABLE_SOFTWARE_TPG = enable_software_tpg,
                    RUN_NUMBER = run_number,
                    SUPERCHUNK_FACTOR = superchunk_factor,
                    EMU_FANOUT = emu_fanout,
                    DATA_FILE = data_file,
                    TPG_CHANNEL_MAP = tpg_channel_map,
                    CONNECTIONS_FILE=connections_file,
                    DATA_SOURCE = source_data,
                    WIBULATOR_DATA = wibulator_data,
                    UHAL_LOG_LEVEL = uhal_log_level,
                    OUTPUT_PATH = output_path,
                ))

        print(f"'{json_file}' generation completed.")

    cli()
    
