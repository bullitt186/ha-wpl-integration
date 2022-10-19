
import re
import datetime
import pandas as pd

PATTERN_ELSTERTABLE = r'\s*{\s*\"(\w+)\"\s*,\s*(\w+)\s*,\s*(\w+)\s*}'
PATTERN_ERRORLIST = r'\s*{\s*(\w+)\s*,\s*"(.+)"\s*}'

# ID of the sender to use
can_sender_id = ["0xe2", "0x00"]

# IDs of the clients to test
can_client_ids = [["0x31", "0x00"], ["0xa1", "0x00"], ["0x91", "0x00"]]

# Load text
with open("/Users/bullitt/SynologyDrive/IoT/ha-wpl-integration/resources/ElsterTable.inc", "r") as elstertable_file:
	elstertable_lines = elstertable_file.readlines()
# Put all the lines into a single string
elstertable = "".join(elstertable_lines)

# Find all the elstertable matches
elstertable_matches = re.findall(PATTERN_ELSTERTABLE, elstertable, flags=re.MULTILINE)
# Extract the relevant match information
can_signals = [ [et_m[1], et_m[0], et_m[2]] for et_m in elstertable_matches ]

# Create a DF for the can signals
#can_signals_df = pd.DataFrame(data=can_signals)
# Rename the columns
#can_signals_df.columns = ["Index", "Name", "Type"]
# Export it as a CSV
#can_signals_df.to_csv("elstertable.csv", index=False)


# Find all the errorlist matches
errorlist_matches = re.findall(PATTERN_ERRORLIST, elstertable, flags=re.MULTILINE)
# Extract the relevant match information
errorlist = [ [el_m[0], el_m[1]] for el_m in errorlist_matches ]

# Create a DF for the error list
#errorlist_df = pd.DataFrame(data=errorlist)
# Rename the columns
#errorlist_df.columns = ["Index", "Name"]
# Export it as a CSV
#errorlist_df.to_csv("errorlist.csv", index=False)


# This array holds the bytes of the can message
can_msg = ["0x00", "0x00", "0x00", "0x00", "0x00", "0x00", "0x00"]
all_requests = ""

# Build request statements
for can_client_id in can_client_ids:
    can_msg[0] = can_client_id[0]
    can_msg[1] = can_client_id[1]
    for can_signal in can_signals:
        high_byte = can_signal[0][2:4]
        low_byte = can_signal[0][4:6]
        if high_byte == 00:
            can_msg[2] = "0x" + low_byte
            can_msg[3] = "0x00"
            can_msg[4] = "0x00"
            can_msg[5] = "0x00"
            can_msg[6] = "0x00"
        else:
            can_msg[2] = "0xfa"
            can_msg[3] = "0x" + high_byte
            can_msg[4] = "0x" + low_byte
            can_msg[5] = "0x00"
            can_msg[6] = "0x00"
        
        single_request = "// Request " + can_signal[1] + " (" + can_signal[0] + ") from " +  can_msg[0] + " " + can_msg[1] + "\n"
        single_request += "id(send_state)[0]=" + can_msg[0] + ";id(send_state)[1]="  + can_msg[1] + ";id(send_state)[2]="  + can_msg[2] + ";id(send_state)[3]="  + can_msg[3] + ";id(send_state)[4]="  + can_msg[4] + ";id(send_state)[5]="  + can_msg[5] + ";id(send_state)[6]="  + can_msg[6] + ";\n" 
        single_request += "id(update_sensor).publish_state(true);\nid(update_sensor).publish_state(false);\n\n"
        all_requests += single_request

print(all_requests)




        


