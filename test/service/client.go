package service

import (
	"bytes"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"time"

	"github.com/apex/log"

	"github.com/jeremyhahn/cropdroid-room/response"
)

type Client struct {
	endpoint string
	client   *http.Client
}

func NewClient(endpoint string) *Client {
	return &Client{
		client: &http.Client{
			Timeout: time.Second * 10,
		},
		endpoint: endpoint}
}

func (c *Client) State() (*response.State, error) {
	url := fmt.Sprintf("%s/state", c.endpoint)
	log.Debug(url)
	reader, err := c.doRequest(url)
	if err != nil {
		return nil, err
	}
	state, err := response.NewState(reader)
	if err != nil {
		return nil, err
	}
	return state, nil
}

func (c *Client) Switch(channel, position int) (*response.Switch, error) {
	url := fmt.Sprintf("%s/switch/%d/%d", c.endpoint, channel, position)
	log.Debug(url)
	reader, err := c.doRequest(url)
	if err != nil {
		return nil, err
	}
	_switch, err := response.NewSwitch(reader)
	if err != nil {
		return nil, err
	}
	return _switch, nil
}

func (c *Client) TimerSwitch(channel, seconds int) (*response.TimerSwitch, error) {
	url := fmt.Sprintf("%s/timer/%d/%d", c.endpoint, channel, seconds)
	log.Debug(url)
	reader, err := c.doRequest(url)
	if err != nil {
		return nil, err
	}
	timerSwitch, err := response.NewTimerSwitch(reader)
	if err != nil {
		return nil, err
	}
	return timerSwitch, nil
}

func (c *Client) doRequest(url string) (io.Reader, error) {
	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return nil, err
	}
	resp, err := c.client.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	b, err := ioutil.ReadAll(resp.Body)
	log.Debugf("doRequest response: %s", string(b))
	return bytes.NewBuffer(b), nil
}
