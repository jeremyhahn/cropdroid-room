package response

import (
	"encoding/json"
	"fmt"
	"io"
)

type State struct {
	Metrics  map[string]float64 `json:"metrics"`
	Channels []int              `json:"channels"`
}

func NewState(reader io.Reader) (*State, error) {
	var state State
	decoder := json.NewDecoder(reader)
	if err := decoder.Decode(&state); err != nil {
		return nil, err
	}
	return &state, nil

}

func (s *State) String() string {
	return fmt.Sprintf("metrics=%v+, channels=%v+",
		s.Metrics, s.Channels)
}
