package response

import (
	"encoding/json"
	"fmt"
	"io"
)

type Switch struct {
	Channel  int `json:"channel"`
	Pin      int `json:"pin"`
	Position int `json:"position"`
}

func NewSwitch(reader io.Reader) (*Switch, error) {
	s := &Switch{}
	decoder := json.NewDecoder(reader)
	if err := decoder.Decode(s); err != nil {
		return nil, err
	}
	return s, nil

}

func (s *Switch) String() string {
	return fmt.Sprintf("channel=%d, pin=%d, position=%d",
		s.Channel, s.Pin, s.Position)
}
