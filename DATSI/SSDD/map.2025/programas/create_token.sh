#!/bin/bash
# CREA UN FICHERO VACÍO EN /tmp CON LA FECHA Y HORA ACTUAL
touch /tmp/$USER-$(date --iso-8601=ns).txt
