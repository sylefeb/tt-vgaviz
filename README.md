# TinyTapeout VGA trace visualizer

This tool can help you visually check the VGA traces produced by cocotb on your TinyTapeout design, following the VGA PMOD pinout (see https://tinytapeout.com/competitions/demoscene/).

The default resolution is 640x480, expecting the design clock to run at the VGA clock. The VGA resolution can be adjusted in the [CMakeLists.txt](https://github.com/sylefeb/tt-vgaviz/blob/8a4bd219de0a8ebd76b91f7c0f4675eb7dc5f83b/CMakeLists.txt#L25).

## Building the tool

Clone, then enter the repo directory and:
```
mkdir BUILD
cd BUILD
cmake ..
make -j8
```

After completion you can run the tool on the output of a simulation FST trace:
```
./tt-vgaviz {tt_project_directory}/test/tb.fst
```

## Getting the trace

To obtain a trace, refer to the TinyTapout simulation README, e.g. [here](https://github.com/TinyTapeout/tt08-verilog-template/blob/main/test/README.md) for tt08.

My process has been as follows:

### 1) Install the PDK
```
pip install volare
export PDK_ROOT=~/.volare/
volare enable --pdk sky130 cd1748bb197f9b7af62a54507de6624e30363943
```

### 2) Simulation

I am using the following `test.py`. Note that it takes a long time and does not really implement any test, so it is not well suited to push to be used in the tt08 github workflow. Its main goal is to produce the trace to check the VGA signal.

```python
import cocotb
from cocotb.clock import Clock
from cocotb.triggers import ClockCycles


@cocotb.test()
async def test_project(dut):
    dut._log.info("Start")

    # Set the clock period to 40 ns (25 MHz)
    clock = Clock(dut.clk, 40, units="ns")
    cocotb.start_soon(clock.start())

    # Reset
    dut._log.info("Reset")
    dut.ena.value = 1
    dut.ui_in.value = 0
    dut.uio_in.value = 0
    dut.rst_n.value = 0
    await ClockCycles(dut.clk, 10)
    dut.rst_n.value = 1

    dut._log.info("Test project")

    # Wait for one clock cycle to see the output values
    await ClockCycles(dut.clk, 1)

    prev_vs = '0'
    frames = 0
    ticks = 0
    while frames < 2:
        ticks = ticks + 1
        # wait one cycle
        await ClockCycles(dut.clk, 1)
        # check vertical sync
        if dut.uo_out.value.binstr[7-3] == '1' and prev_vs == '0':
            print("vsynch on ",ticks)
        if dut.uo_out.value.binstr[7-3] == '0' and prev_vs == '1':
            print("vsynch off ",ticks)
            frames = frames + 1
        prev_vs = dut.uo_out.value.binstr[7-3]
```

Run the simulation with
```
make -B WAVES=1
```
The `WAVES=1` is because we want the tool to produce a trace in the FST format, which is way more efficient than VCD.

The file is output as `tb.fst`. You can then run the tool using `./tt-vgaviz {tt_project_directory}/test/tb.fst`.

### 3) Gate level simulation

To run the gate level simulation, download the `tt_submission.zip` artifact from the GDS action summary, then copy the file from inside it `tt_submission/{your_module_name}.v` as `gate_level_netlist.v`.

Execute 
```
make -B GATES=yes WAVES=1
```

Wait until it ends (takes a long time). You can again run the tool with `./tt-vgaviz {tt_project_directory}/test/tb.fst`.

Be patient, it can take several minutes to see the picture appear (as long as it is alive the tool outputs dots to the console).

## I see nothing !?!

Use `gtkwave` to visualize the trace and check vertical sync (uo_out[3]) and horzontal sync (uo_out[7]) for activity. Also lookout for any 'x' state, these are usually not a good sign beyond reset.

Hope this will be useful! If you like it, a little tar is welcome :)
