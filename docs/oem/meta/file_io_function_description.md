# Description of file IO function usage

## List of functions related to reading file IO command

- decode_oem_meta_file_io_read_req function
- encode_oem_meta_file_io_read_resp function

## Read file IO command format

| Byte | Type | Request Data | | 0 | uint8 | FileHandle <br> This field is a
handle that is used to identify PLDM command type. | | 1 | enum8 | ReadOption
<br> This field is a read option is used to identify the read file option <br>
See Table 1 for the option. | | 2 | uint8 | Length <br> The length in bytes N of
data being sent in this part in the ReadInfo field. | | Variable | uint8 |
ReadInfo <br> Portion of reading information. <br> There will be different
reading information according to different ReadOption. <br> See Table 2 and
Table 3 for details | | Byte | Type | Response Data | | 0 | int | CompletionCode
<br> value: { 0, -EINVAL, -EOVERFLOW } | | Variable | uint8 | ReadResponse <br>
Portion of reading response. <br> There will be different reading response
according to different ReadOption. <br> See Table 4 and Table 5 for details |

- Table 1 – ReadOption Definition in Read file IO command

| Value | Name | Description | | 0x00 | ReadFileAttribute | Get file size and
checksum | | 0x01 | ReadFileData | Get file data |

- Table 2 – ReadInfo Definition when ReadOption is ReadFileAttribute in Read
  file IO command

| Byte | Type | Request Data | | 0 | - | No request data |

- Table 3 – ReadInfo Definition when ReadOption is ReadFileData in Read file IO
  command

| Byte | Type | Request Data | | 0 | uint8 | Length <br> This field indicates
the length of data that needs to be read. | | 1 | uint8 | TransferFlag <br> The
transfer flag that indiates what part of the transfer this request represents.
<br> Possible values: {Start=0x01, Middle=0x02, End=0x04, StartAndEnd=0x05} | |
2 | uint8 | HighOffset <br> The higher byte in offset. | | 3 | uint8 | LowOffset
<br> The lower byte in offset. |

- Table 4 – ReadResponse Definition when ReadOption is ReadFileAttribute in Read
  file IO command

| Byte | Type | Response Data | | 0:1 | uint16 | This field indicates the size
of the entire file, in bytes. | | 2:5 | uint32 | This field indicates the
checksum of the entire file. |

- Table 5 – ReadResponse Definition when ReadOption is ReadFileData in Read file
  IO command

| Byte | Type | Response Data | | 0 | uint8 | Length <br> This field indicates
the length of data that needs to be read. | | 1 | uint8 | TransferFlag <br> The
transfer flag that indiates what part of the transfer this response represents.
<br> Possible values: {Start=0x01, Middle=0x02, End=0x04, StartAndEnd=0x05} | |
2 | uint8 | HighOffset <br> The higher byte in offset. | | 3 | uint8 | LowOffset
<br> The lower byte in offset. | | Variable | uint8 | FileData <br> File data
can be up to 255 bytes. |
