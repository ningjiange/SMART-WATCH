Get-Process python* -ErrorAction SilentlyContinue | Select-Object Id, ProcessName | Format-Table -AutoSize
