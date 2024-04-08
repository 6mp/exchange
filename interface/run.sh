#!/bin/bash

source venv/bin/activate
pip install -r requirements.txt
python3 -m grpc_tools.protoc -I../protos --python_out=src/pb --pyi_out=src/pb --grpc_python_out=src/pb ../protos/Orderbook.proto
cd src
python3 main.py
