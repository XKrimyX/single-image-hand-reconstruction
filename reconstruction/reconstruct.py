#!/usr/bin/env python3
"""兼容旧配置的重建入口。

实际实现位于 simplehand_reconstruct.py。Qt 旧配置如果仍调用 reconstruction/reconstruct.py，
也会进入同一个命令行流程。
"""

from simplehand_reconstruct import main


if __name__ == "__main__":
    raise SystemExit(main())
