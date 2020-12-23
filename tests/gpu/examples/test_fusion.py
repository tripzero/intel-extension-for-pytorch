import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.testing._internal.common_utils import TestCase
import torch_ipex
import pytest


torch._C._jit_set_profiling_mode(False)
torch._C._jit_set_profiling_executor(False)

cpu_device = torch.device("cpu")
dpcpp_device = torch.device("xpu")


class MatmulSum(torch.nn.Module):
    def __init__(self):
        super(MatmulSum, self).__init__()


    def forward(self, m1, m2, a):
        y = torch.matmul(m1, m2)
        y += a
        return y


class TransMatmulScalePost(torch.nn.Module):
    def __init__(self):
        super(TransMatmulScalePost, self).__init__()

    def forward(self, m1, m2, added):
        return torch.matmul(m1, m2.transpose(-1, -2)) / 8 + added


class Conv2dRelu(torch.nn.Module):
    def __init__(self, in_channels, out_channels, **kwargs):
        super(Conv2dRelu, self).__init__()
        self.conv = nn.Conv2d(in_channels, out_channels, **kwargs)

    def forward(self, x, a):
        return F.relu(self.conv(x) + a, inplace=True)


class Conv2dSigmoid(torch.nn.Module):
    def __init__(self, in_channels, out_channels, **kwargs):
        super(Conv2dSigmoid, self).__init__()
        self.conv = nn.Conv2d(in_channels, out_channels, **kwargs)

    def forward(self, x, a):
        return torch.sigmoid(self.conv(x))


class TestNNMethod(TestCase):
    def test_matmul_sum_fusion(self, dtype=torch.float):
        m1 = torch.randn([4, 2], device=cpu_device)
        m2 = torch.randn([2, 2], device=cpu_device)
        acc = torch.randn([2], device=cpu_device)

        m1_dpcpp = m1.to("dpcpp")
        m2_dpcpp = m2.to("dpcpp")
        acc_dpcpp = acc.to("dpcpp")
        model = MatmulSum()
        raw = model(m1, m2, acc)
        print("raw: ", raw)
        modelJit = torch.jit.script(model)
        with torch.no_grad():
            real = modelJit(m1_dpcpp, m2_dpcpp, acc_dpcpp)
            print("real: ", real.cpu())
        self.assertEqual(raw, real.to(cpu_device))
        del modelJit

    def test_trans_baddbmm_scale_sum_fusion(self, dtype=torch.float):
        m1 = torch.randn((2,2,3), device=cpu_device)
        m2 = torch.randn((2,2,3), device=cpu_device)
        added1 = torch.randn((2,1,1), device=cpu_device)
        added2 = torch.randn((2,2,2), device=cpu_device)

        model = TransMatmulScalePost()
        raw1 = model(m1, m2, added1)
        raw2 = model(m1, m2, added2)
        print("raw1: ", raw1)
        print("raw2: ", raw2)

        m1_dpcpp = m1.to(dpcpp_device)
        m2_dpcpp = m2.to(dpcpp_device)
        added1_dpcpp = added1.to(dpcpp_device)
        added2_dpcpp = added2.to(dpcpp_device)

        modelJit = torch.jit.script(model)
        with torch.no_grad():
            real1 = modelJit(m1_dpcpp, m2_dpcpp, added1_dpcpp)
            real2 = modelJit(m1_dpcpp, m2_dpcpp, added2_dpcpp)
            print("real1:", real1.to(cpu_device))
            print("real2:", real2.to(cpu_device))
        self.assertEqual(raw1, real1.to(cpu_device))
        self.assertEqual(raw2, real2.to(cpu_device))
        del modelJit

    def test_conv_relu_fusion(self, dtype=torch.float):
        x = torch.randn([1, 2, 3, 3], device=cpu_device)
        a1 = torch.ones([1, 2, 1, 1], device=cpu_device)
        a2 = torch.ones([1, 2, 1, 1], device=dpcpp_device)
        a3 = torch.ones([1, 2, 1, 1], device=dpcpp_device)

        a1.fill_(2)
        a3.fill_(2)

        model = Conv2dRelu(2, 2, kernel_size=3, stride=1, bias=True)
        y = model(x, a1)
        print("raw: ", y)

        x = x.to("xpu")
        model.to("xpu")
        modelJit = torch.jit.script(model)
        # modelJit.to("xpu")
        # print(modelJit.graph)
        with torch.no_grad():
            # print(modelJit.graph_for(x, a2))
            y_dpcpp = modelJit(x, a3)
            print("fusion:", y_dpcpp.cpu())
        self.assertEqual(y, y_dpcpp.to(cpu_device))
        del modelJit

    def test_conv_sigmoid_fusion(self, dtype=torch.float):
        x = torch.randn([1, 2, 3, 3], device=cpu_device)
        a1 = torch.ones([1, 2, 1, 1], device=cpu_device)
        a2 = torch.ones([1, 2, 1, 1], device=dpcpp_device)
        a3 = torch.ones([1, 2, 1, 1], device=dpcpp_device)

        a1.fill_(2)
        a3.fill_(2)

        model = Conv2dSigmoid(2, 2, kernel_size=3, stride=1, bias=True)
        y = model(x, a1)
        print("raw: ", y)

        x = x.to("xpu")
        model.to("xpu")
        modelJit = torch.jit.script(model)
        with torch.no_grad():
            y_dpcpp = modelJit(x, a3)
            print("fusion:", y_dpcpp.cpu())
        self.assertEqual(y, y_dpcpp.to(cpu_device))
        del modelJit