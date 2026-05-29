Get-PnpDevice -Status OK | Where-Object { $_.Class -eq 'USB' -or $_.Class -eq 'Ports' } | Select-Object Class, FriendlyName | Format-Table -AutoSize
