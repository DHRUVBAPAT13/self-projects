package clcacheapi

import (
	"testing"
	"time"
)

func TestCreateCache(t *testing.T) {
	interval := time.Millisecond * 40
	cache := NewCache(interval)
	if cache.cache == nil {
		t.Errorf("cache is nil")
	}
}

func TestAddGetCache(t *testing.T) {
	interval := time.Millisecond * 40
	cache := NewCache(interval)

	cases := []struct {
		inputKey   string
		inputValue []byte
	}{
		{
			inputKey:   "key1",
			inputValue: []byte("value1"),
		},
		{
			inputKey:   "key2",
			inputValue: []byte("value2"),
		},
		{
			inputKey:   "key3",
			inputValue: []byte("value3"),
		},
	}

	for _, cas := range cases {
		cache.Add(cas.inputKey, cas.inputValue)
	}

	actual, ok := cache.Get("key1")
	if !ok {
		t.Errorf("key1 not found in cache")
	}
	if string(actual) != "value1" {
		t.Errorf("%s doesn't match %s", string(actual), "value1")
	}
}

func TestReapCache(t *testing.T) {

	interval := time.Millisecond * 40
	cache := NewCache(interval)

	keyOne := "key1"
	cache.Add(keyOne, []byte("value1"))

	time.Sleep(interval + time.Millisecond*2)

	_, ok := cache.Get(keyOne)
	if ok {
		t.Errorf("%s should have been reaped", keyOne)
	}

}

func TestReapFail(t *testing.T) {

	interval := time.Millisecond * 40
	cache := NewCache(interval)

	keyOne := "key1"
	cache.Add(keyOne, []byte("value1"))

	time.Sleep(interval / 2)

	_, ok := cache.Get(keyOne)
	if !ok {
		t.Errorf("%s should not have been reaped", keyOne)
	}

}
