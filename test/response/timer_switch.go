package response

import (
	"encoding/json"
	"fmt"
	"io"
)

type TimerSwitch struct {
	Channel  int `json:"channel"`
	Duration int `json:"duration"`
}

func NewTimerSwitch(reader io.Reader) (*TimerSwitch, error) {
	var ts TimerSwitch
	decoder := json.NewDecoder(reader)
	if err := decoder.Decode(&ts); err != nil {
		return nil, err
	}
	return &ts, nil

}

func (s *TimerSwitch) String() string {
	return fmt.Sprintf("channel=%d, duration=%d",
		s.Channel, s.Duration)
}
