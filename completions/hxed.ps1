# hxed.ps1

Register-ArgumentCompleter -CommandName hxed -ScriptBlock {
    param($commandName, $parameterName, $wordToComplete, $commandAst, $fakeBoundParameters)

    $options = @(
        "-f","--file",
        "-m","--mode",
        "-hm","--heatmap",
        "-w","--width",
        "-g","--grouping",
        "-a","--ascii",
        "-c","--color",
        "-s","--string",
        "-e","--entropy",
        "-th","--toggle-header",
        "-sz","--skip-zero",
        "-re","--reverse",
        "-o","--offset",
        "-l","--limit",
        "-r","--read-size",
        "-se","--search",
        "-p","--pager",
        "-ro","--raw",
        "--show-config",
        "-h","--help",
        "-v","--version"
    )

    $modes = @("0","1","2","3","hex","bin","oct","dec")
    $heatmaps = @("adaptiv","fixed","none")
    $searchPrefixes = @("a:","x:","d:","b:")

    $elements = @($commandAst.CommandElements | ForEach-Object { $_.Extent.Text })
    $prev = $null

    if ($elements.Count -ge 2) {
        if ($wordToComplete -and $elements[-1] -eq $wordToComplete) {
            $prev = $elements[-2]
        }
        else {
            $prev = $elements[-1]
        }
    }

    switch ($wordToComplete) {
        { $_ -match "^-" } {
            $options | Where-Object { $_ -like "$wordToComplete*" } | ForEach-Object {
                [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterName', $_)
            }
            return
        }
    }

    switch ($prev) {
        "-m" { $modes | ForEach-Object {
            [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)
        }; return }
        "--mode" { $modes | ForEach-Object {
            [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)
        }; return }

        "-hm" { $heatmaps | ForEach-Object {
            [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)
        }; return }
        "--heatmap" { $heatmaps | ForEach-Object {
            [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)
        }; return }

        "-se" { $searchPrefixes | ForEach-Object {
            [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)
        }; return }
        "--search" { $searchPrefixes | ForEach-Object {
            [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)
        }; return }
    }

    # fallback: Dateien
    Get-ChildItem -Name | Where-Object { $_ -like "$wordToComplete*" } | ForEach-Object {
        [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)
    }
}
