Get-CimInstance Win32_PnPEntity | Where-Object {$_.PNPClass -eq 'Ports'} | Select-Object Name | Format-Table -AutoSize
