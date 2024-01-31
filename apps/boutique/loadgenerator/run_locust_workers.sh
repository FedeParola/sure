#!/bin/bash

workers=$1
shift

locust --worker --processes $workers &
locust --master $@
