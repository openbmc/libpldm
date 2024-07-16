# Description of file IO messages

## Read Message Format

### Table 1. Read Request

| Offset   | Type  | Name       | Description                                                                                                                                                  |
| -------- | ----- | ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 0        | uint8 | Handle     | This field is a handle that is used to identify PLDM command type.                                                                                           |
| 1        | enum8 | ReadOption | This field is a read option is used to identify the read file option. <br> See Table 3 for the option.                                                       |
| 2        | uint8 | Length     | The length in bytes N of data being sent in this part in the ReadInfo field.                                                                                 |
| Variable | uint8 | ReadInfo   | Portion of reading information. <br> There will be different reading information according to different ReadOption. <br> See Table 4 and Table 5 for details |

### Table 2. Read response

| Offset   | Type  | Name           | Description                                                                                                                                            |
| -------- | ----- | -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 0        | int   | CompletionCode | value: { PLDM_SUCCESS, PLDM_ERROR_INVALID_DATA, PLDM_ERROR_INVALID_LENGTH }                                                                            |
| Variable | uint8 | ReadResponse   | Portion of reading response. <br> There will be different reading response according to different ReadOption. <br> See Table 6 and Table 7 for details |

### Table 3. ReadOption of message type

| Value | Name              | Description                |
| ----- | ----------------- | -------------------------- |
| 0x00  | ReadFileAttribute | Get file size and checksum |
| 0x01  | ReadFileData      | Get file data              |

### Table 4. ReadInfo Definition when ReadOption is ReadFileAttribute in message type

| Offset | Type | Name | Description     |
| ------ | ---- | ---- | --------------- |
| 0      | -    | -    | No request data |

### Table 5. ReadInfo Definition when ReadOption is ReadFileData in message type

| Offset | Type  | Name         | Description                                                                                                                                                    |
| ------ | ----- | ------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0      | uint8 | Length       | This field indicates the length of data that needs to be read.                                                                                                 |
| 1      | uint8 | TransferFlag | The transfer flag that indiates what part of the transfer this request represents. <br> Possible values: {Start=0x01, Middle=0x02, End=0x04, StartAndEnd=0x05} |
| 2      | uint8 | HighOffset   | The higher byte in offset.                                                                                                                                     |
| 3      | uint8 | LowOffset    | The lower byte in offset.                                                                                                                                      |

### Table 6. ReadResponse Definition when ReadOption is ReadFileAttribute in message type

| Offset | Type   | Name     | Description                                                 |
| ------ | ------ | -------- | ----------------------------------------------------------- |
| 0:1    | uint16 | Size     | This field indicates the size of the entire file, in bytes. |
| 2:5    | uint32 | Checksum | This field indicates the checksum of the entire file.       |

### Table 7. ReadResponse Definition when ReadOption is ReadFileData in message type

| Offset   | Type  | Name         | Description                                                                                                                                                     |
| -------- | ----- | ------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0        | uint8 | Length       | This field indicates the length of data that needs to be read.                                                                                                  |
| 1        | uint8 | TransferFlag | The transfer flag that indiates what part of the transfer this response represents. <br> Possible values: {Start=0x01, Middle=0x02, End=0x04, StartAndEnd=0x05} |
| 2        | uint8 | HighOffset   | The higher byte in offset.                                                                                                                                      |
| 3        | uint8 | LowOffset    | The lower byte in offset.                                                                                                                                       |
| Variable | uint8 | FileData     | File data can be up to 255 bytes.                                                                                                                               |
