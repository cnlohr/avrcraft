#!/bin/bash
ls -l $1 | awk '{ print $5}'
