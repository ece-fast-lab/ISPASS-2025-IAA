#!/usr/bin/env bash

# Script configure IAA devices

# Usage : ./configure_iaa_user <mode> <start,end> <wq_size>
# mode: 0 - shared, 1 - dedicated
# devices: 0 - all devices or start and end device number.
# For example,	1, 7 will configure all the Socket0 devices in host or 0, 3 will configure all the Socket0 devices in guest
#		9, 15 will configure all the Socket1 devices and son on
#		1 will configure only device 1
# wq_size: 1-128
#
# select iax config
#
dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#echo ${dir}
#
# count iax instances
#
iax_dev_id="0cfe"
num_iax=$(lspci -d:${iax_dev_id} | wc -l)
echo "Found ${num_iax} IAX instances"

dedicated=${1:-0}; shift
device_num=${1:-$num_iax}; shift
wq_size=${1:-128}; shift

if [ ${dedicated} -eq 0 ]; then
	mode="shared"
else
	mode="dedicated"
fi

#set first,step counters to correctly enumerate iaa devices
first=1 && step=2

#
# disable iax wqs and devices
#
echo "Disable IAX"

for ((i = ${first}; i < ${step} * ${num_iax}; i += ${step})); do
	echo disable wq iax${i}/wq${i}.0 >& /dev/null
	accel-config disable-wq iax${i}/wq${i}.0 >& /dev/null
	# echo disable wq iax${i}/wq${i}.1 >& /dev/null
	# accel-config disable-wq iax${i}/wq${i}.1 >& /dev/null
	echo disable iax iax${i} >& /dev/null
	accel-config disable-device iax${i} >& /dev/null
done
echo "Configuring devices: ${device_num}"

if [ ${device_num} == $num_iax ]; then
	echo "Configuring all devices"
	start=${first}
	end=$(( ${step} * ${num_iax} ))
else
	echo "Configuring devices ${device_num}"
	declare -a array=($(echo ${device_num}| tr "," " "))
	start=${array[0]}
	if [ ${array[1]} ];then
		end=$((${array[1]} + 1 ))
	else
		end=$((${array[0]} + 1 ))
	fi
fi
#
# enable all iax devices and wqs
#
echo "Enable IAX ${start} to ${end}"
for ((i = ${start}; i < ${end}; i += ${step})); do
	# Config Engines and groups

	accel-config config-engine iax${i}/engine${i}.0 --group-id=0
	accel-config config-engine iax${i}/engine${i}.1 --group-id=0
	accel-config config-engine iax${i}/engine${i}.2 --group-id=0
	accel-config config-engine iax${i}/engine${i}.3 --group-id=0
	accel-config config-engine iax${i}/engine${i}.4 --group-id=0
	accel-config config-engine iax${i}/engine${i}.5 --group-id=0
	accel-config config-engine iax${i}/engine${i}.6 --group-id=0
	accel-config config-engine iax${i}/engine${i}.7 --group-id=0


	# Config WQ: group 0, size = 128, priority=10, mode=shared, type = user, name=iax_crypto, threashold=128, block_on_fault=1, driver_name=user
	accel-config config-wq iax${i}/wq${i}.0 -g 0 -s $wq_size -p 10 -m ${mode} -y user -n user${i} -t $wq_size -b 1 -d user

	# Enable one more workqueue
	# accel-config config-wq iax${i}/wq${i}.1 -g 0 -s $wq_size -p 10 -m ${mode} -y user -n user${i} -t $wq_size -b 1 -d user
	#accel-config config-wq iax${i}/wq${i}.0 -g 0 -s $wq_size -p 10 -m ${mode} -y user -n user${i} -b 1 -d user

	echo enable device iax${i}
	accel-config enable-device iax${i}
	echo enable wq iax${i}/wq${i}.0
	accel-config enable-wq iax${i}/wq${i}.0

	# Enable one more workqueue
	# echo enable wq iax${i}/wq${i}.1
	# accel-config enable-wq iax${i}/wq${i+1}.1
done
