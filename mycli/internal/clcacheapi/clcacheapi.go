package clcacheapi

import "time"

type Cache struct {
	// Define your cache fields here
	cache map[string]cacheEntry
}

type cacheEntry struct {
	// Define your cache entry fields here
	val       []byte
	createdAt time.Time
}

func NewCache(interval time.Duration) Cache {
	c := Cache{
		cache: make(map[string]cacheEntry),
	}
	go c.Reaploop(interval)
	return c
}

func (c *Cache) Add(key string, val []byte) {
	c.cache[key] = cacheEntry{
		val:       val,
		createdAt: time.Now(),
	}
}

func (c *Cache) Get(key string) ([]byte, bool) {
	cacheE, ok := c.cache[key]
	if !ok {
		return nil, false
	}
	return cacheE.val, true
}

func (c *Cache) Reap(interval time.Duration) {
	timeout := time.Now().Add(-interval)
	for k, v := range c.cache {
		if v.createdAt.Before(timeout) {
			delete(c.cache, k)
		}
	}
}

func (c *Cache) Reaploop(interval time.Duration) {
	ticker := time.NewTicker(interval)
	for range ticker.C {
		c.Reap(interval)
	}
}

//	TODO: start of from 1:17 hours in the video, after the tests
