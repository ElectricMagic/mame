// license:BSD-3-Clause
// copyright-holders:Mike Balfour
/***************************************************************************

    redalert.c, Irem M27 hardware

    If you have any questions about how this driver works, don't hesitate to
    ask.  - Mike Balfour (mab22@po.cwru.edu)

    ********************************************************************
    IREM 'DEMONEYE-X' proto 1981

    proto sound board

    8910
    6821   8910
    6802  sound6 sound7
        3.579545MHz

     main board M-27M-C

      11.73MHz              6502            x x x xx
                                             x x x  on
                      4116            8
                      4116            -
                      4116            9
                      4116            6
                      4116            A
                      4116            7
                      4116            B
                      4116

     sub board 1 M-27Sb

      1a2

      2114
      2114
                  2114 <- two parts piggy-backed
                  2114 <- two parts piggy-backed
                  2114 2114
                  2114 2114

    sub board 2 M-42-S

      1a       clr(missing)

                      2114
                      2114
                      2114
                      2114
                      2114
                      2114

    *********************************************************************

    Known issues/to-do's all games:
        * Video timing from schematics

    Known issues/to-do's Red Alert:
        * Analog sounds
        * DIP switches have a different meaning in test mode (see manual)
        * Audio CPU NMI is generated by a 74121 multivibrator, the correct
          pulse length is not emulated

    Known issues/to-do's Demoneye-X:
        * Game is NOT_WORKING due to missing graphics layer
        * Everything needs to be verified on real PCB or schematics

    Known issues/to-do's Panther:
        * Sound comms doesn't work
        * No title screen?
		* Fails ROM check in service mode with "ROM ERR 0", bootleg/prototype set?
		* Likewise sports bad wording in input test

    ********************************************************************
    IREM 'WW III' 1981

    From readme (Stefan Lindberg)

    The PCB is not working so I don't know if the roms are fine, the sound rom
    was for sure bad it gave different checksums but most of the reads matched
    the MAME soundrom (red alert) it is marked exactly the same "w3s1"(IC5).
    The Bprom matched the Red Alert set also... marked "W3" i think?
    it's hard to see because the sticker has been damaged.
    The other eproms except one did not match anything in MAME,
    and only one of those had the eprom type markings on it... I read all
    like that type.

     Board set consists of:
        M-27MB (Main board)
        M-27SC
        M-37B  (Sound board)
        M-33 SUB-1

    ********************************************************************
    Panther notes:
	- Hold start 1 on boot, press coin chutes or service button to cycle:
	-> RAM/ROM check 
	-> Continous Video drawing check (only if above is success)
	-> (NMI again goes to PROM check and beyond)
	- Hold start 2 on boot: 
	-> PROM check?
	-> Input check
	-> Freeze
	- Notes on "ROM ERR 0":
	  PC=b482 ROM check main routine
	  PC=b5cc SUM16 individual ROM chunk (ROM 0 -> 8000-87ff, ROM 1 -> 8800-8fff ...)
	  PC=b5b4 Taking the branch -> failed check
	  

****************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "includes/redalert.h"


#define MAIN_PCB_CLOCK      (XTAL(12'500'000))
#define MAIN_CPU_CLOCK      (MAIN_PCB_CLOCK / 16)



/*************************************
 *
 *  Interrupt generation
 *
 *************************************/

INTERRUPT_GEN_MEMBER(redalert_state::redalert_vblank_interrupt)
{
	// TODO: use PORT_CHANGED instead
	if( ioport("COIN")->read() )
		/* the service coin as conntected to the CPU's RDY pin as well */
		device.execute().pulse_input_line(INPUT_LINE_NMI, attotime::zero);

	device.execute().set_input_line(M6502_IRQ_LINE, ASSERT_LINE);
}


uint8_t redalert_state::redalert_interrupt_clear_r()
{
	if (!machine().side_effects_disabled())
		m_maincpu->set_input_line(M6502_IRQ_LINE, CLEAR_LINE);

	/* the result never seems to be actually used */
	return m_screen->vpos();
}



void redalert_state::redalert_interrupt_clear_w(uint8_t data)
{
	m_maincpu->set_input_line(M6502_IRQ_LINE, CLEAR_LINE);
}

uint8_t redalert_state::panther_interrupt_clear_r()
{
	if (!machine().side_effects_disabled())
		m_maincpu->set_input_line(M6502_IRQ_LINE, CLEAR_LINE);

	return ioport("VOLUM")->read();
}

uint8_t redalert_state::panther_unk_r()
{
	return ((machine().rand() & 0x01) | (ioport("KEY2")->read() & 0xfe));
}

/*************************************
 *
 *  Memory handlers
 *
 *************************************/

void redalert_state::redalert_main_map(address_map &map)
{
	map(0x0000, 0x1fff).ram();
	map(0x2000, 0x3fff).ram().w(FUNC(redalert_state::redalert_bitmap_videoram_w)).share("bitmap_videoram");
	map(0x4000, 0x4fff).ram().share("charram");
	map(0x5000, 0xbfff).rom();
	map(0xc000, 0xc000).mirror(0x0f8f).portr("DSW").nopw();
	map(0xc010, 0xc010).mirror(0x0f8f).portr("KEY1").nopw();
	map(0xc020, 0xc020).mirror(0x0f8f).portr("KEY2").nopw();
	map(0xc030, 0xc030).mirror(0x0f8f).nopr().w(FUNC(redalert_state::redalert_audio_command_w));
	map(0xc040, 0xc040).mirror(0x0f8f).nopr().writeonly().share("video_control");
	map(0xc050, 0xc050).mirror(0x0f8f).nopr().writeonly().share("bitmap_color");
	map(0xc060, 0xc060).mirror(0x0f8f).nopr().w(FUNC(redalert_state::redalert_voice_command_w));
	map(0xc070, 0xc070).mirror(0x0f8f).rw(FUNC(redalert_state::redalert_interrupt_clear_r), FUNC(redalert_state::redalert_interrupt_clear_w));
	map(0xf000, 0xffff).rom().region("maincpu", 0x8000);
}

void redalert_state::ww3_main_map(address_map &map)
{
	map(0x0000, 0x1fff).ram();
	map(0x2000, 0x3fff).ram().w(FUNC(redalert_state::redalert_bitmap_videoram_w)).share("bitmap_videoram");
	map(0x4000, 0x4fff).ram().share("charram");
	map(0x5000, 0xbfff).rom();
	map(0xc000, 0xc000).mirror(0x0f8f).portr("DSW").nopw();
	map(0xc010, 0xc010).mirror(0x0f8f).portr("KEY1").nopw();
	map(0xc020, 0xc020).mirror(0x0f8f).portr("KEY2").nopw();
	map(0xc030, 0xc030).mirror(0x0f8f).nopr().w(FUNC(redalert_state::redalert_audio_command_w));
	map(0xc040, 0xc040).mirror(0x0f8f).nopr().writeonly().share("video_control");
	map(0xc050, 0xc050).mirror(0x0f8f).nopr().writeonly().share("bitmap_color");
	map(0xc070, 0xc070).mirror(0x0f8f).rw(FUNC(redalert_state::redalert_interrupt_clear_r), FUNC(redalert_state::redalert_interrupt_clear_w));
	map(0xf000, 0xffff).rom().region("maincpu", 0x8000);
}

void redalert_state::panther_main_map(address_map &map)
{
	map(0x0000, 0x1fff).ram();
	map(0x2000, 0x3fff).ram().w(FUNC(redalert_state::redalert_bitmap_videoram_w)).share("bitmap_videoram");
	map(0x4000, 0x4fff).ram().share("charram");
	map(0x5000, 0xbfff).rom();
	map(0xc000, 0xc000).mirror(0x0f8f).portr("DSW").nopw();
	map(0xc010, 0xc010).mirror(0x0f8f).portr("KEY1").nopw();
	map(0xc020, 0xc020).mirror(0x0f8f).r(FUNC(redalert_state::panther_unk_r)); /* vblank? */
	map(0xc030, 0xc030).mirror(0x0f8f).nopr().w(FUNC(redalert_state::redalert_audio_command_w));
	map(0xc040, 0xc040).mirror(0x0f8f).nopr().writeonly().share("video_control");
	map(0xc050, 0xc050).mirror(0x0f8f).nopr().writeonly().share("bitmap_color");
	map(0xc070, 0xc070).mirror(0x0f8f).rw(FUNC(redalert_state::panther_interrupt_clear_r), FUNC(redalert_state::redalert_interrupt_clear_w));
	map(0xf000, 0xffff).rom().region("maincpu", 0x8000);
}

void redalert_state::demoneye_main_map(address_map &map)
{
	map(0x0000, 0x1fff).ram();
	map(0x2000, 0x3fff).ram().w(FUNC(redalert_state::redalert_bitmap_videoram_w)).share("bitmap_videoram");
	map(0x4000, 0x5fff).ram().share("charram");
	map(0x6000, 0xbfff).rom();
	map(0xc000, 0xc000).mirror(0x0f8f).portr("DSW").nopw();
	map(0xc010, 0xc010).mirror(0x0f8f).portr("KEY1").nopw();
	map(0xc020, 0xc020).mirror(0x0f8f).portr("KEY2").nopw();
	map(0xc030, 0xc030).mirror(0x0f8f).nopr().w(FUNC(redalert_state::demoneye_audio_command_w));
	map(0xc040, 0xc040).mirror(0x0f8f).nopr().writeonly().share("video_control");
	map(0xc050, 0xc050).mirror(0x0f8f).nopr().writeonly().share("bitmap_color");
	map(0xc060, 0xc060).mirror(0x0f80).noprw();   /* unknown */
	map(0xc061, 0xc061).mirror(0x0f80).noprw();   /* unknown */
	map(0xc062, 0xc062).mirror(0x0f80).noprw();   /* unknown */
	map(0xc070, 0xc070).mirror(0x0f8f).rw(FUNC(redalert_state::redalert_interrupt_clear_r), FUNC(redalert_state::redalert_interrupt_clear_w)); /* probably not correct */
	map(0xf000, 0xffff).rom().region("maincpu", 0x8000);
}



/*************************************
 *
 *  Port definitions
 *
 *************************************/

static INPUT_PORTS_START( redalert )
	PORT_START("DSW")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW:1,2")
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet in Service Mode" ) PORT_DIPLOCATION("SW:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SW:4")
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x08, "7000" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Coinage ) ) PORT_DIPLOCATION("SW:5,6")
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("SW:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_HIGH, "SW:8" )

	PORT_START("KEY1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )    /* pin 35 - N.C. */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )    /* pin 36 - N.C. */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )   /* Meter */

	PORT_START("KEY2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )   /* Meter */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )    /* pin 33 - N.C. */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )    /* pin 34 - N.C. */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )   /* Meter */

	PORT_START("COIN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) PORT_IMPULSE(1)
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( panther )
	PORT_START("DSW")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW:1,2")
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet in Service Mode" ) PORT_DIPLOCATION("SW:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SW:4")
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x08, "7000" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Coinage ) ) PORT_DIPLOCATION("SW:5,6")
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("SW:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_HIGH, "SW:8" )

	PORT_START("KEY1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 ) /* pin 35 - N.C. */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 ) /* pin 36 - N.C. */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON4 ) /* Meter */

	PORT_START("KEY2")
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Meter */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Meter */

	PORT_START("COIN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) PORT_IMPULSE(1)
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("VOLUM")
	// vertical lever according to cabinet pic, no centering
	// TODO: more akin to AD_STICK_Z? Verify in-game ranges
	PORT_BIT( 0xff, 0x80, IPT_POSITIONAL_V ) PORT_SENSITIVITY(70) PORT_KEYDELTA(3) PORT_CENTERDELTA(0)
INPUT_PORTS_END

static INPUT_PORTS_START( demoneye )
	PORT_START("DSW")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x08, "7000" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START("KEY1")
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Meter */

	PORT_START("KEY2")
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Meter */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Meter */

	PORT_START("COIN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) PORT_IMPULSE(1)
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

void redalert_state::redalert(machine_config &config)
{
	/* basic machine hardware */
	M6502(config, m_maincpu, MAIN_CPU_CLOCK);
	m_maincpu->set_addrmap(AS_PROGRAM, &redalert_state::redalert_main_map);
	m_maincpu->set_vblank_int("screen", FUNC(redalert_state::redalert_vblank_interrupt));

	/* video hardware */
	redalert_video(config);

	/* audio hardware */
	redalert_audio(config);
}

void redalert_state::ww3(machine_config &config)
{
	/* basic machine hardware */
	M6502(config, m_maincpu, MAIN_CPU_CLOCK);
	m_maincpu->set_addrmap(AS_PROGRAM, &redalert_state::ww3_main_map);
	m_maincpu->set_vblank_int("screen", FUNC(redalert_state::redalert_vblank_interrupt));

	/* video hardware */
	ww3_video(config);

	/* audio hardware */
	ww3_audio(config);
}

void redalert_state::panther(machine_config &config)
{
	/* basic machine hardware */
	M6502(config, m_maincpu, MAIN_CPU_CLOCK);
	m_maincpu->set_addrmap(AS_PROGRAM, &redalert_state::panther_main_map);
	m_maincpu->set_vblank_int("screen", FUNC(redalert_state::redalert_vblank_interrupt));

	/* video hardware */
	panther_video(config);

	/* audio hardware */
	ww3_audio(config);
}

void redalert_state::demoneye(machine_config &config)
{
	/* basic machine hardware */
	M6502(config, m_maincpu, MAIN_CPU_CLOCK);
	m_maincpu->set_addrmap(AS_PROGRAM, &redalert_state::demoneye_main_map);
	m_maincpu->set_vblank_int("screen", FUNC(redalert_state::redalert_vblank_interrupt));

	/* video hardware */
	demoneye_video(config);

	/* audio hardware */
	demoneye_audio(config);
}



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( panther )
	ROM_REGION( 0x10000, "maincpu", 0 )
	// TODO: marked as BAD_DUMP since these all fails ROM check
	// we need a second set to counter-check exact identification
	ROM_LOAD( "qr-1.bin",      0x8000, 0x0800, BAD_DUMP CRC(406dc606) SHA1(c12b91145aa579813b7b0e8eb7933bf35e4a5b97) )
	ROM_LOAD( "qr-2.bin",      0x8800, 0x0800, BAD_DUMP CRC(e7e64b11) SHA1(0fcfbce552b22edce9051b6fad0974f81ab44973) )
	ROM_LOAD( "qr-3.bin",      0x9000, 0x0800, BAD_DUMP CRC(dfec33f2) SHA1(4e631a3a8c7873e8f51a81e8b73704729269ee01) )
	ROM_LOAD( "qr-4.bin",      0x9800, 0x0800, CRC(60571aa0) SHA1(257474383ad7cb90e9e4f9236b3f865a991d688a) )
	ROM_LOAD( "qr-5.bin",      0xa000, 0x0800, CRC(2ac19b54) SHA1(613a800179f9705df03967889eb23ef71baed493) )
	ROM_LOAD( "qr-6.bin",      0xa800, 0x0800, BAD_DUMP CRC(02fbd9d9) SHA1(65b5875c78886b51c9bdfc75e730b9f67ce72cfc) )
	ROM_LOAD( "qr-7.bin",      0xb000, 0x0800, BAD_DUMP CRC(b3e2d6cc) SHA1(7bb18f17d635196e617e8f68bf8d866134c362d1) )

	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "q7a.bin",       0x7000, 0x0800, CRC(febd1674) SHA1(e122d0855ab6a352d741f9013c20ec31e0068248) )

	ROM_REGION( 0x0200, "proms", 0 ) /* color PROM */
	ROM_LOAD( "6349-1j-8026.1a",      0x0000, 0x0200, CRC(ea9c2ada) SHA1(cb720c0d77b24f995e0750b3fa42a68962c7a977) ) /* 512*8 74S472 or compatible BPROM like a 82s147 */
ROM_END


ROM_START( ww3 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "w3i5.3f",      0x5000, 0x1000, CRC(9fc24ad3) SHA1(697ab22555ff5aae09f50051ccda545c17a0ac8a) )
	ROM_LOAD( "w3i6.3d",      0x6000, 0x1000, CRC(cb2a308c) SHA1(9f3bc22bad31165e080e81d4a3fb0ec2aad235fe) )
	ROM_LOAD( "w3i7b.3b",     0x7000, 0x1000, CRC(1a0c3936) SHA1(fa2d0a1425624ae4d811adc9ea75850641207682) )
	ROM_LOAD( "w3i8.3g",      0x8000, 0x1000, CRC(9e18a92c) SHA1(c352b7c66ebfc875bb44772874e58c5f6d8cabbc) )
	ROM_LOAD( "w3i9.3e",      0x9000, 0x1000, CRC(8c5884a4) SHA1(e6f5a5e65d9e59ff37385ab02852c1fdce9088db) )
	ROM_LOAD( "w3ia.3c",      0xa000, 0x1000, CRC(dccb8605) SHA1(f4c5e1a5de0828c5e39f37e2bf10f4f60bef856a) )
	ROM_LOAD( "w3ib.3a",      0xb000, 0x1000, CRC(3658e465) SHA1(2c910b2e9d689cb577d8a63bc4d07d0770a6de68) )

	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "w3s1",         0x7000, 0x0800, CRC(4af956a5) SHA1(25368a40d7ebc60316fd2d78ec4c686e701b96dc) )

	ROM_REGION( 0x0200, "proms", 0 ) /* color PROM */
	ROM_LOAD( "m-27sc.1a",    0x0000, 0x0200, CRC(b1aca792) SHA1(db37f99b9880cc3c434e2a55a0bbb017d9a72aa3) ) /* 512*8 74S472 or compatible BPROM like a 82s147 */
ROM_END

ROM_START( redalert )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rag5",         0x5000, 0x1000, CRC(d7c9cdd6) SHA1(5ff5cdceaa00083b745cf5c74b096f7edfadf737) )
	ROM_LOAD( "rag6",         0x6000, 0x1000, CRC(cb2a308c) SHA1(9f3bc22bad31165e080e81d4a3fb0ec2aad235fe) )
	ROM_LOAD( "rag7n",        0x7000, 0x1000, CRC(82ab2dae) SHA1(f8328b048384afac245f1c16a2d0864ffe0b4741) )
	ROM_LOAD( "rag8n",        0x8000, 0x1000, CRC(b80eece9) SHA1(d986449bdb1d94832187c7f953f01330391ef4c9) )
	ROM_LOAD( "rag9",         0x9000, 0x1000, CRC(2b7d1295) SHA1(1498af0c55bd38fe79b91afc38921085102ebbc3) )
	ROM_LOAD( "ragab",        0xa000, 0x1000, CRC(ab99f5ed) SHA1(a93713bb03d61cce64adc89b874b67adea7c53cd) )
	ROM_LOAD( "ragb",         0xb000, 0x1000, CRC(8e0d1661) SHA1(bff4ddca761ddd70113490f50777e62c66813685) )

	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "w3s1",         0x7000, 0x0800, CRC(4af956a5) SHA1(25368a40d7ebc60316fd2d78ec4c686e701b96dc) )

	ROM_REGION( 0x10000, "voice", 0 )
	ROM_LOAD( "ras1b",        0x0000, 0x1000, CRC(ec690845) SHA1(26a84738bd45ed21dac6c8383ebd9c3b9831024a) )
	ROM_LOAD( "ras2",         0x1000, 0x1000, CRC(fae94cfc) SHA1(2fd798706bb3afda3fb55bc877e597cc4e5d0c15) )
	ROM_LOAD( "ras3",         0x2000, 0x1000, CRC(20d56f3e) SHA1(5c32ee3365407e6d3f7ab5662e9ecbac437ed4cb) )
	ROM_LOAD( "ras4",         0x3000, 0x1000, CRC(130e66db) SHA1(385b8f889fee08fddbb2f75a691af569109eacd1) )

	ROM_REGION( 0x0200, "proms", 0 ) /* color PROM */
	ROM_LOAD( "m-257sc.1a",   0x0000, 0x0200, CRC(b1aca792) SHA1(db37f99b9880cc3c434e2a55a0bbb017d9a72aa3) ) /* 512*8 74S472 or compatible BPROM like a 82s147 */
ROM_END


ROM_START( demoneye )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "demoneye.6",   0x6000, 0x1000, CRC(b03ee3a9) SHA1(66b6115fbb4e8097152702022c59c464e8211e5a) )
	ROM_LOAD( "demoneye.7",   0x7000, 0x1000, CRC(667a5de7) SHA1(c3ce7fbbc6c98250e9d5f85854e6887017ca5ff9) )
	ROM_LOAD( "demoneye.8",   0x8000, 0x1000, CRC(257484d7) SHA1(3937cce546462a471adbdc1da63ddfc20cfc7b79) )
	ROM_LOAD( "demoneye.9",   0x9000, 0x1000, CRC(bd8d79a8) SHA1(68c1443ef78b545eb9e612573b86515c3ad7f103) )
	ROM_LOAD( "demoneye.a",   0xa000, 0x1000, CRC(a27d08aa) SHA1(659ad22778e852fc58f3951d62bc01151c973d36) )
	ROM_LOAD( "demoneye.b",   0xb000, 0x1000, CRC(1fd3585b) SHA1(b1697b7b21b739499fda1e155530dbfab89f3358) )

	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "demoneye.7s",  0x2000, 0x1000, CRC(8fdc9364) SHA1(3fccb5b22f08d6a0cde85863c1ce5399c84f233e) )
	ROM_LOAD( "demoneye.6s",  0x3000, 0x1000, CRC(0a23def9) SHA1(b52f52be312ec7810e3c9cbd3913e887f983b1ee) )

	ROM_REGION( 0x0200, "proms", 0 ) /* color PROM */
	ROM_LOAD( "demoneye.1a2", 0x0000, 0x0200, CRC(eaf5a66e) SHA1(d8ebe05ba5d75fbf6ad45f710e5bd27b6afad44b) ) /* 512*8 74S472 or compatible BPROM like a 82s147 */

	ROM_REGION( 0x0200, "user1", 0 ) /* unknown */
	ROM_LOAD( "demoneye.1a",  0x0000, 0x0200, CRC(d03488ea) SHA1(11027f502ad2a9255b2e5611ab2eee16ede1d704) ) /* 512*8 74S472 or compatible BPROM like a 82s147 */
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1981, panther,  0,        panther,  panther,  redalert_state, empty_init, ROT270, "Irem",               "Panther (bootleg?)",    MACHINE_NO_SOUND | MACHINE_SUPPORTS_SAVE )
GAME( 1981, redalert, 0,        redalert, redalert, redalert_state, empty_init, ROT270, "Irem (GDI license)", "Red Alert",  MACHINE_IMPERFECT_SOUND | MACHINE_SUPPORTS_SAVE )
GAME( 1981, ww3,      redalert, ww3,      redalert, redalert_state, empty_init, ROT270, "Irem",               "WW III",     MACHINE_IMPERFECT_SOUND | MACHINE_SUPPORTS_SAVE )
GAME( 1981, demoneye, 0,        demoneye, demoneye, redalert_state, empty_init, ROT270, "Irem",               "Demoneye-X", MACHINE_NOT_WORKING | MACHINE_IMPERFECT_GRAPHICS | MACHINE_IMPERFECT_SOUND | MACHINE_SUPPORTS_SAVE )
