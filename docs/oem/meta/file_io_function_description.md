# Description of file IO function usage

## List of read file IO functions

- decode_oem_meta_file_io_read_req function

## decode_oem_meta_file_io_read_req function format

<table>
<thead>
<tr>
<th>Byte</th>
<th>Type</th>
<th>Request Data</th>
</tr>
</thead>
<tbody>
<tr>
<td>0</td>
<td>uint8</td>
<td>Handle<br>This field is a handle that is used to 
identify PLDM command type.</td>
</tr>
<tr>
<td>1</td>
<td>enum8</td>
<td>ReadOption<br>This field is a read option is used to 
identify the read file option<br>See Table 1 for the option.</td>
</tr>
<tr>
<td>2</td>
<td>uint8</td>
<td>Length<br>The length in bytes N of data being sent in this 
part in the ReadInfo field.</td>
</tr>
<tr>
<td>Variable</td>
<td>uint8</td>
<td>ReadInfo<br>Portion of reading information.<br>There will be 
different reading information according to different ReadOption.<br>
See Table 2 and Table 3 for details</td>
</tr>
<tr>
<td>Byte</td>
<td>Type</td>
<td>Response Data</td>
</tr>
<tr>
<td>0</td>
<td>int</td>
<td>CompletionCode<br>value: { 0, -EINVAL, -EOVERFLOW }</td>
</tr>
<tr>
<td>Variable</td>
<td>uint8</td>
<td>ReadResponse<br>Portion of reading response.<br>There will be 
different reading response according to different ReadOption. <br>
See Table 4 and Table 5 for details</td>
</tr>
</tbody>
</table>

- Table 1 – ReadOption of decode_oem_meta_file_io_read_req function

<table>
<thead>
<tr>
<th>Value</th>
<th>Name</th>
<th>Description</th>
</tr>
</thead>
<tbody>
<tr>
<td>0x00</td>
<td>ReadFileAttribute</td>
<td>Handle<br>Get file size and checksum</td>
</tr>
<tr>
<td>0x01</td>
<td>ReadFileData</td>
<td>Get file data</td>
</tr>
</tbody>
</table>

- Table 2 – ReadInfo Definition when ReadOption is ReadFileAttribute in
  decode_oem_meta_file_io_read_req function

<table>
<thead>
<tr>
<th>Byte</th>
<th>Type</th>
<th>Request Data</th>
</tr>
</thead>
<tbody>
<tr>
<td>0</td>
<td>-</td>
<td>No request data</td>
</tr>
</tbody>
</table>

- Table 3 – ReadInfo Definition when ReadOption is ReadFileData in
  decode_oem_meta_file_io_read_req function

<table>
<thead>
<tr>
<th>Byte</th>
<th>Type</th>
<th>Request Data</th>
</tr>
</thead>
<tbody>
<tr>
<td>0</td>
<td>uint8</td>
<td>Length<br>This field indicates the length of data that needs to be read.</td>
</tr>
<tr>
<td>1</td>
<td>uint8</td>
<td>TransferFlag<br>The transfer flag that indiates what part of the transfer 
this request represents.<br>Possible values: {Start=0x01, Middle=0x02, End=0x04, 
StartAndEnd=0x05}</td>
</tr>
<tr>
<td>2</td>
<td>uint8</td>
<td>HighOffset<br>The higher byte in offset.</td>
</tr>
<tr>
<td>3</td>
<td>uint8</td>
<td>LowOffset<br>The lower byte in offset.</td>
</tr>
</tbody>
</table>

- Table 4 – ReadResponse Definition when ReadOption is ReadFileAttribute in
  decode_oem_meta_file_io_read_req function

<table>
<thead>
<tr>
<th>Byte</th>
<th>Type</th>
<th>Response Data</th>
</tr>
</thead>
<tbody>
<tr>
<td>0:1</td>
<td>uint16</td>
<td>This field indicates the size of the entire file, in bytes.</td>
</tr>
<tr>
<td>2:5</td>
<td>uint32</td>
<td>This field indicates the checksum of the entire file.</td>
</tr>
</tbody>
</table>

- Table 5 – ReadResponse Definition when ReadOption is ReadFileData in
  decode_oem_meta_file_io_read_req function

<table>
<thead>
<tr>
<th>Byte</th>
<th>Type</th>
<th>Response Data</th>
</tr>
</thead>
<tbody>
<tr>
<td>0</td>
<td>uint8</td>
<td>Length<br>This field indicates the length of data that needs to be read.</td>
</tr>
<tr>
<td>1</td>
<td>uint8</td>
<td>TransferFlag<br>The transfer flag that indiates what part of the transfer this 
response represents.<br>Possible values: {Start=0x01, Middle=0x02, End=0x04, 
StartAndEnd=0x05}</td>
</tr>
<tr>
<td>2</td>
<td>uint8</td>
<td>HighOffset<br>The higher byte in offset.</td>
</tr>
<tr>
<td>3</td>
<td>uint8</td>
<td>LowOffset<br>The lower byte in offset.</td>
</tr>
<tr>
<td>Variable</td>
<td>uint8</td>
<td>FileData<br>File data can be up to 255 bytes.</td>
</tr>
</tbody>
</table>
