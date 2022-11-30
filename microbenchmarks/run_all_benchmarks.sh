pathHostF=~/hostsRennes.txt

hostFile=${pathHostF} bash run_control_frames_dyn.sh
hostFile=${pathHostF} bash run_control_frames_tot.sh

hostFile=${pathHostF} bash run_single_down_unidir_dyn.sh
hostFile=${pathHostF} bash run_single_down_unidir_tot.sh

hostFile=${pathHostF} bash run_single_up_unidir_dyn.sh
hostFile=${pathHostF} bash run_single_up_unidir_tot.sh

hostFile=${pathHostF} bash run_pairs_unidir.sh
hostFile=${pathHostF} bash run_pairs_bidir.sh

hostFile=${pathHostF} bash run_simplemix_setup.sh