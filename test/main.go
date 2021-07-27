package main

import (
	"errors"
	"flag"
	"time"

	"github.com/apex/log"

	"github.com/jeremyhahn/cropdroid-room/service"
)

const (
	CHANNEL_SIZE = 6
	SWITCH_OFF   = 0
	SWITCH_ON    = 1

	TIMER_TEST_SECONDS = 3
)

var (
	ErrGetState = errors.New("error getting state")
)

func main() {
	debug := flag.Bool("debug", false, "Debug flag")
	endpoint := flag.String("host", "http://192.168.0.91", "IP or hostname of controller")

	flag.Parse()

	if *debug {
		log.SetLevel(log.DebugLevel)
	}

	if *endpoint == "" {
		log.Fatal("--endpoint parameter required")
	}

	log.Infof("Initializing HTTP ECR client...")
	client := service.NewClient(*endpoint)

	runSwitchTests(client)
	runTimerSwitchTests(client)

	log.Info("Test Successful!")
}

func runSwitchTests(client *service.Client) {
	log.Infof("Switching on %d channels on...", CHANNEL_SIZE)
	for i := 0; i < CHANNEL_SIZE; i++ {
		log.Infof("Switching on channel %d", i)
		_switch, err := client.Switch(i, SWITCH_ON)
		if err != nil {
			log.WithError(err).Info(_switch.String())
			continue
		}
	}

	state, err := client.State()
	if err != nil {
		log.WithError(err).Fatal(ErrGetState.Error())
	}
	for i := 0; i < CHANNEL_SIZE; i++ {
		if state.Channels[i] != SWITCH_ON {
			log.Fatalf("Channel %d expected to be on, but was found off", i)
		}
	}

	log.Infof("Switching off %d channels off...", CHANNEL_SIZE)
	for i := 0; i < CHANNEL_SIZE; i++ {
		log.Infof("Switching on channel %d", i)
		_switch, err := client.Switch(i, SWITCH_OFF)
		if err != nil {
			log.WithError(err).Info(_switch.String())
			continue
		}
	}
}

func runTimerSwitchTests(client *service.Client) {
	for i := 0; i < CHANNEL_SIZE; i++ {
		log.Infof("Switching on channel %d for %d seconds", i, TIMER_TEST_SECONDS)
		timerSwitch, err := client.TimerSwitch(i, TIMER_TEST_SECONDS)
		if err != nil {
			log.WithError(err).Info(timerSwitch.String())
			continue
		}

		state, err := client.State()
		if err != nil {
			log.WithError(err).Fatal(ErrGetState.Error())
		}
		if state.Channels[i] != SWITCH_ON {
			log.Fatalf("Channel %d expected to be on, but was found off", i)
		}

		time.Sleep(2 * time.Second)
		state, err = client.State()
		if err != nil {
			log.WithError(err).Fatal(ErrGetState.Error())
		}
		if state.Channels[i] != SWITCH_OFF {
			log.Fatalf("Channel %d expected to be off, but was found on", i)
		}
	}
}
