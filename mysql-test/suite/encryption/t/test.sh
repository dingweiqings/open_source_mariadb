#!/bin/bash  
for i in {1..10000}  
do  
    echo "$i;`openssl rand -hex 32`"  >> filekeys-data-too-large.key
done  



