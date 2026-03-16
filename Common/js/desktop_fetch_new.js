var _scriptSrc = document.currentScript && document.currentScript.src;

Module['instantiateWasm'] = function(imports, successCallback) {
    // 1. Replicate the original OnlyOffice "IsLocal" detection
    function internal_isLocal() {
        if (window.navigator && window.navigator.userAgent.toLowerCase().indexOf("ascdesktopeditor") < 0)
            return false;
        if (window.location && window.location.protocol == "file:")
            return true;
        if (window.document && window.document.currentScript && 0 == window.document.currentScript.src.indexOf("file:///"))
            return true;
        return false;
    }

    var wasmAbsPath = _scriptSrc.replace(/\.js(\?.*)?$/, '.wasm').substr(7);

    console.log("Loading WASM from:", wasmAbsPath);

    if (!wasmAbsPath) {
        console.error("Could not determine wasm absolute path; falling back to Emscripten default.");
        return false;
    }



    console.log("RESULT OF LOCAL TEST:" + internal_isLocal())
    if (internal_isLocal()) {
        // Use the custom desktop protocol path
        var wasmPath = "ascdesktop://fonts/" + wasmAbsPath;

        var xhr = new XMLHttpRequest();
        xhr.open('GET', wasmPath, true);
        xhr.responseType = 'arraybuffer';

        // Keep the original MIME/Charset overrides
        if (xhr.overrideMimeType)
            xhr.overrideMimeType('text/plain; charset=x-user-defined');
        else
            xhr.setRequestHeader('Accept-Charset', 'x-user-defined');

        xhr.onload = function() {
            console.log("XHR onload — status:", xhr.status, "byteLength:", xhr.response && xhr.response.byteLength);
            
            if (xhr.status === 200 || (xhr.status === 0 && xhr.response && xhr.response.byteLength > 0)) {
                console.log("Calling WebAssembly.instantiate...");
                WebAssembly.instantiate(xhr.response, imports)
                    .then(function(result) {
                        console.log("WASM instantiated successfully, calling successCallback");
                        successCallback(result.instance, result.module);
                    })
                    .catch(function(e) {
                        console.error("WASM instantiation failed:", e);
                    });
            } else {
                console.error("XHR status check failed — status:", xhr.status, "response:", xhr.response);
            }
        };

        xhr.onerror = function() {
            console.error("XHR onerror fired for:", wasmPath);
        };

        xhr.ontimeout = function() {
            console.error("XHR timed out for:", wasmPath);
        };

        xhr.onreadystatechange = function() {
            console.log("XHR readyState:", xhr.readyState, "status:", xhr.status);
        };
        xhr.send(null);
        return {}; // Tell Emscripten instantiation is happening asynchronously
    }

    // 3. Fallback for standard Web
    // Returning false tells Emscripten to use its default fetch/instantiate logic
    return false; 
};

