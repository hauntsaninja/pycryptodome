from typing import Union, Dict, Iterable

from Crypto.Cipher._mode_ecb import EcbMode
from Crypto.Cipher._mode_cbc import CbcMode
from Crypto.Cipher._mode_cfb import CfbMode
from Crypto.Cipher._mode_ofb import OfbMode
from Crypto.Cipher._mode_ctr import CtrMode
from Crypto.Cipher._mode_openpgp import OpenPgpMode
from Crypto.Cipher._mode_eax import EaxMode

CASTMode = int

MODE_ECB: CASTMode
MODE_CBC: CASTMode
MODE_CFB: CASTMode
MODE_OFB: CASTMode
MODE_CTR: CASTMode
MODE_OPENPGP: CASTMode
MODE_EAX: CASTMode

def new(key: bytes | bytearray,
        mode: CASTMode,
        iv : bytes | bytearray | None = ...,
        IV : bytes | bytearray | None = ...,
        nonce : bytes | bytearray | None = ...,
        segment_size : int = ...,
        mac_len : int = ...,
        initial_value : int | bytes | bytearray = ...,
        counter : Dict = ...) -> \
        Union[EcbMode, CbcMode, CfbMode, OfbMode, CtrMode, OpenPgpMode]: ...

block_size: int
key_size : Iterable[int]
