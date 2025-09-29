/*
 * From QEMU WASM Demo
 * https://github.com/ktock/qemu-wasm-demo
*/

if (typeof Module === 'undefined') {
    Module = {};
}
Module['arguments'] = [
    '-machine', 'q35', 
    '-cpu', 'qemu64', 
    '-m', '1G',
    '-L', '/pack-rom/',
    '-serial', 'mon:stdio',
    '-drive', 'file=/tmp/AstralOS.qcow2',
    '-drive', 'if=pflash,format=raw,unit=1,file=/writable/OVMF_VARS-pure-efi.fd',
    '-drive', 'if=pflash,format=raw,unit=0,file=/uefi/OVMF_CODE-pure-efi.fd,readonly=on',
    '-net', 'none'
];
Module['locateFile'] = function(path, prefix) {
    return '/AstralOS/image/' + path;
};
Module['mainScriptUrlOrBlob'] = '/AstralOS/image/out.js'