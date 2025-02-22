// license:BSD-3-Clause
// copyright-holders:hap
// thanks-to:Sean Riddle
/*******************************************************************************

CXG Computachess II (CXG-002 or WA-002)

It's the sequel to Sensor Computachess, on similar hardware. The chess engine is
again by Intelligent Software.

Hardware notes:
- PCB label: W&A 002B-600-003
- Hitachi 44840A14 MCU @ ~650kHz (62K resistor)
- buzzer, 16 leds, button sensors chessboard

There's also an older revision (002 600 002 PCB, separate LED PCB), the rest of
the hardware is the same. Seen with either A13 or A14 MCU.

HD44840A13/A14 MCU is used in:
- CXG Computachess II
- CXG Advanced Portachess (white or red version)
- CGL Computachess 2
- CGL Grandmaster Sensory 2
- CGL Computachess Travel Sensory
- Hanimex Computachess II (HCG 1600)
- Hanimex Computachess III (HCG 1700)
- Schneider Sensor Chessmaster MK 5

It's not known yet how to differentiate between the 2 program revisions. Versions
with a higher serial number are more likely to have the A14 MCU.

*******************************************************************************/

#include "emu.h"

#include "cpu/hmcs40/hmcs40.h"
#include "machine/sensorboard.h"
#include "sound/dac.h"
#include "video/pwm.h"

#include "speaker.h"

// internal artwork
#include "cxg_cpchess2.lh"


namespace {

class cpchess2_state : public driver_device
{
public:
	cpchess2_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_board(*this, "board"),
		m_display(*this, "display"),
		m_dac(*this, "dac"),
		m_inputs(*this, "IN.%u", 0)
	{ }

	void cpchess2(machine_config &config);

	// New Game button is directly tied to MCU reset
	DECLARE_INPUT_CHANGED_MEMBER(reset_button) { m_maincpu->set_input_line(INPUT_LINE_RESET, newval ? ASSERT_LINE : CLEAR_LINE); }

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	// devices/pointers
	required_device<hmcs40_cpu_device> m_maincpu;
	required_device<sensorboard_device> m_board;
	required_device<pwm_display_device> m_display;
	required_device<dac_1bit_device> m_dac;
	required_ioport_array<2> m_inputs;

	u8 m_inp_mux = 0;

	template<int N> void mux_w(u8 data);
	void control_w(u16 data);
	u16 input_r();
};

void cpchess2_state::machine_start()
{
	save_item(NAME(m_inp_mux));
}



/*******************************************************************************
    I/O
*******************************************************************************/

template<int N>
void cpchess2_state::mux_w(u8 data)
{
	// R2x,R3x: input mux, led data
	const u8 shift = N * 4;
	m_inp_mux = (m_inp_mux & ~(0xf << shift)) | ((data ^ 0xf) << shift);
	m_display->write_mx(m_inp_mux);
}

void cpchess2_state::control_w(u16 data)
{
	// D4: speaker out
	m_dac->write(BIT(data, 4));

	// D2,D3: led select
	m_display->write_my(~data >> 2 & 3);
}

u16 cpchess2_state::input_r()
{
	u16 data = 0;

	// D6,D7: read buttons
	for (int i = 0; i < 2; i++)
		if (m_inp_mux & m_inputs[i]->read())
			data |= 0x40 << i;

	// D8-D15: read chessboard
	for (int i = 0; i < 8; i++)
		if (BIT(m_inp_mux, i))
			data |= m_board->read_file(i ^ 7) << 8;

	return ~data;
}



/*******************************************************************************
    Input Ports
*******************************************************************************/

static INPUT_PORTS_START( cpchess2 )
	PORT_START("IN.0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_NAME("King")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_NAME("Queen")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_NAME("Rook")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_NAME("Bishop")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_NAME("Knight")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_NAME("Pawn")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("IN.1")
	PORT_BIT(0x0f, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_T) PORT_NAME("Take Back")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_R) PORT_NAME("Reverse Play")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_S) PORT_NAME("Sound")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_L) PORT_NAME("Level")

	PORT_START("RESET")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_N) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(cpchess2_state::reset_button), 0) PORT_NAME("New Game")
INPUT_PORTS_END



/*******************************************************************************
    Machine Configs
*******************************************************************************/

void cpchess2_state::cpchess2(machine_config &config)
{
	// basic machine hardware
	HD44840(config, m_maincpu, 650'000); // approximation
	m_maincpu->write_r<2>().set(FUNC(cpchess2_state::mux_w<0>));
	m_maincpu->write_r<3>().set(FUNC(cpchess2_state::mux_w<1>));
	m_maincpu->write_d().set(FUNC(cpchess2_state::control_w));
	m_maincpu->read_d().set(FUNC(cpchess2_state::input_r));

	SENSORBOARD(config, m_board).set_type(sensorboard_device::BUTTONS);
	m_board->init_cb().set(m_board, FUNC(sensorboard_device::preset_chess));
	m_board->set_delay(attotime::from_msec(150));

	// video hardware
	PWM_DISPLAY(config, m_display).set_size(2, 8);
	config.set_default_layout(layout_cxg_cpchess2);

	// sound hardware
	SPEAKER(config, "speaker").front_center();
	DAC_1BIT(config, m_dac).add_route(ALL_OUTPUTS, "speaker", 0.25);
}



/*******************************************************************************
    ROM Definitions
*******************************************************************************/

ROM_START( cpchess2 )
	ROM_REGION( 0x4000, "maincpu", 0 )
	ROM_LOAD("1982_nc201_newcrest_44840a14", 0x0000, 0x4000, CRC(c3d9c1e0) SHA1(4185b717a3b6fe916cc438fbdce35dcf32cab825) )
ROM_END

} // anonymous namespace



/*******************************************************************************
    Drivers
*******************************************************************************/

//    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     CLASS           INIT        COMPANY, FULLNAME, FLAGS
SYST( 1982, cpchess2, 0,      0,      cpchess2, cpchess2, cpchess2_state, empty_init, "CXG Systems / White and Allcock / Intelligent Software", "Computachess II", MACHINE_SUPPORTS_SAVE )
