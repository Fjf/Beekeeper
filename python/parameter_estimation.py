import os
import re
import subprocess

import numpy as np
import optuna
from matplotlib import pyplot as plt
from optuna.structs import TrialState


class Configuration(object):
    def __init__(self, params=None, e=1):
        self.algorithm = "mm"
        self.mcts_constant = 0
        self.mcts_prioritization = 0
        self.time = 0.01
        self.first_play_urgency = 0
        self.evaluation_function = e
        self.params = params

    def to_cmd(self, player1=True):
        params = []
        if self.params:
            params = [
                "-m", self.params[0],
                "-q", self.params[1],
                "-u", self.params[2],
                "-d", self.params[3]
            ]

        if player1:
            prio = "-P" if self.mcts_prioritization is True else ""
            fpu = "-F" if self.first_play_urgency is True else ""
            result = ["-A", self.algorithm, prio, fpu, "-C", self.mcts_constant, "-T", self.time, "-E",
                    self.evaluation_function] + params
        else:
            prio = "-p" if self.mcts_prioritization is True else ""
            fpu = "-f" if self.first_play_urgency is True else ""
            result = ["-a", self.algorithm, prio, fpu, "-c", self.mcts_constant, "-t", self.time, "-e",
                    self.evaluation_function] + params

        return [str(e) for e in result]

    def __repr__(self):
        return "%s|%s|%d|%s|%d|%s" % (
            self.algorithm, self.mcts_constant, self.mcts_prioritization, self.time, self.first_play_urgency,
            self.evaluation_function)


def playout(p1: Configuration, p2: Configuration):
    base = ["../c/cmake-build-debug/hive_run"]
    command = base + p1.to_cmd() + p2.to_cmd(False)

    result = subprocess.check_output(command).decode("utf-8")
    for line in result.split("\n"):
        if "won" in line:
            if "1" in line:
                return 1
            elif "2" in line:
                return 0
            else:
                return 0.5

    return 0.5


def objective(trial):
    base = Configuration()

    m = trial.suggest_float('movement', 0, 10)
    q = trial.suggest_float('queen', 0, 10)
    u = trial.suggest_float('unused_tiles', 0, 10)
    d = trial.suggest_float('distance_to_queen', 0, 10)

    n_playouts = 100
    result = 0

    for i in range(n_playouts // 2):
        result += playout(base, Configuration((m, q, u, d), e=3))
        result += (1 - playout(Configuration((m, q, u, d), e=3), base))

    return result / n_playouts


def main():
    study = optuna.create_study(direction='maximize')
    study.optimize(objective, n_trials=100)

    pruned_trials = study.get_trials(deepcopy=False, states=[TrialState.PRUNED])
    complete_trials = study.get_trials(deepcopy=False, states=[TrialState.COMPLETE])

    print("Study statistics: ")
    print("  Number of finished trials: ", len(study.trials))
    print("  Number of pruned trials: ", len(pruned_trials))
    print("  Number of complete trials: ", len(complete_trials))

    print("Best trial:")
    trial = study.best_trial

    print("  Value: ", trial.value)

    print("  Params: ")
    for key, value in trial.params.items():
        print("    {}: {}".format(key, value))


def plot():
    data = """[I 2021-06-10 17:17:20,835] Trial 0 finished with value: 0.45 and parameters: {'movement': 6.15325296159549, 'queen': 5.883305602003094, 'unused_tiles': 0.8351727331520675, 'movement_rest': 9.204844174699723, 'queen_rest': 3.542417163899608, 'unused_tiles_rest': 3.69675763905433}. Best is trial 0 with value: 0.45.
[I 2021-06-10 17:19:52,477] Trial 1 finished with value: 0.425 and parameters: {'movement': 7.180811311429962, 'queen': 4.685361701962597, 'unused_tiles': 8.631025580360618, 'movement_rest': 8.857850767888507, 'queen_rest': 8.489512556907417, 'unused_tiles_rest': 0.18181215194080336}. Best is trial 0 with value: 0.45.
[I 2021-06-10 17:22:31,894] Trial 2 finished with value: 0.47 and parameters: {'movement': 4.151286320176014, 'queen': 0.8526552500063211, 'unused_tiles': 7.66806595967611, 'movement_rest': 9.296329285826614, 'queen_rest': 1.2012666355968094, 'unused_tiles_rest': 5.386100674929997}. Best is trial 2 with value: 0.47.
[I 2021-06-10 17:25:14,862] Trial 3 finished with value: 0.5 and parameters: {'movement': 4.403372828884978, 'queen': 0.8855697441890475, 'unused_tiles': 5.589694427285766, 'movement_rest': 5.598982123355885, 'queen_rest': 0.5608667335339657, 'unused_tiles_rest': 2.9060739333993792}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:27:49,332] Trial 4 finished with value: 0.46 and parameters: {'movement': 0.5744820805254192, 'queen': 0.33774656517671753, 'unused_tiles': 8.057754194917024, 'movement_rest': 6.095238863671767, 'queen_rest': 9.515425750930909, 'unused_tiles_rest': 6.947627531376147}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:30:31,967] Trial 5 finished with value: 0.455 and parameters: {'movement': 3.8394217585417802, 'queen': 2.830724984066718, 'unused_tiles': 4.2022492266262566, 'movement_rest': 3.8327520213657684, 'queen_rest': 5.48327848740282, 'unused_tiles_rest': 3.949538415491621}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:33:11,517] Trial 6 finished with value: 0.44 and parameters: {'movement': 7.419706791085869, 'queen': 5.01548070373487, 'unused_tiles': 9.630021548991468, 'movement_rest': 2.5908177486198913, 'queen_rest': 7.852648530591643, 'unused_tiles_rest': 4.090102783608405}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:35:53,195] Trial 7 finished with value: 0.47 and parameters: {'movement': 6.355191511772197, 'queen': 0.05531206133625455, 'unused_tiles': 6.7936054833900315, 'movement_rest': 8.89056422274457, 'queen_rest': 9.493293901134937, 'unused_tiles_rest': 3.9624779198339786}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:38:19,774] Trial 8 finished with value: 0.45 and parameters: {'movement': 8.960761622273687, 'queen': 1.2190878879054767, 'unused_tiles': 9.379760279600712, 'movement_rest': 3.3388671538168344, 'queen_rest': 5.4423952932259105, 'unused_tiles_rest': 2.4052545667562497}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:41:00,745] Trial 9 finished with value: 0.485 and parameters: {'movement': 6.804661311952329, 'queen': 2.256894018933596, 'unused_tiles': 4.956785973354027, 'movement_rest': 3.9391035093908213, 'queen_rest': 1.4968047308079457, 'unused_tiles_rest': 2.2700405643622634}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:42:56,275] Trial 10 finished with value: 0.28 and parameters: {'movement': 1.6424924583264313, 'queen': 8.86442299925209, 'unused_tiles': 1.9591008015449987, 'movement_rest': 0.3031847773159244, 'queen_rest': 0.18148190875265424, 'unused_tiles_rest': 9.565475895187003}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:45:27,619] Trial 11 finished with value: 0.45 and parameters: {'movement': 2.795407060179688, 'queen': 2.7755644308736076, 'unused_tiles': 4.4098433276703215, 'movement_rest': 6.265392047266496, 'queen_rest': 2.2189493887072262, 'unused_tiles_rest': 0.808194164224147}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:48:07,052] Trial 12 finished with value: 0.485 and parameters: {'movement': 9.955158791364212, 'queen': 2.7429910363415524, 'unused_tiles': 5.7108835508720395, 'movement_rest': 5.471264654218423, 'queen_rest': 0.05675141377082393, 'unused_tiles_rest': 1.6452197679955298}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:50:46,251] Trial 13 finished with value: 0.43 and parameters: {'movement': 5.2265577102128455, 'queen': 2.1152528802417545, 'unused_tiles': 3.007335387053343, 'movement_rest': 1.4152626400908312, 'queen_rest': 2.8552559838329543, 'unused_tiles_rest': 2.1706567519030306}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:53:27,449] Trial 14 finished with value: 0.47 and parameters: {'movement': 8.546913795248344, 'queen': 6.389639140012285, 'unused_tiles': 5.698897552052783, 'movement_rest': 7.253262873310577, 'queen_rest': 1.316585181137067, 'unused_tiles_rest': 5.803566435542571}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:56:00,849] Trial 15 finished with value: 0.44 and parameters: {'movement': 4.650148196279858, 'queen': 3.9309871419019786, 'unused_tiles': 2.9536542838071878, 'movement_rest': 4.363745790412543, 'queen_rest': 3.829464898069741, 'unused_tiles_rest': 2.519353248049617}. Best is trial 3 with value: 0.5.
[I 2021-06-10 17:58:47,080] Trial 16 finished with value: 0.465 and parameters: {'movement': 2.5508447310171842, 'queen': 1.5563084042896662, 'unused_tiles': 6.353021029771105, 'movement_rest': 7.330153335633497, 'queen_rest': 1.1088657868618381, 'unused_tiles_rest': 1.2633460334145115}. Best is trial 3 with value: 0.5.
[I 2021-06-10 18:01:24,355] Trial 17 finished with value: 0.465 and parameters: {'movement': 9.7263413075989, 'queen': 3.753509532280259, 'unused_tiles': 6.904694990863839, 'movement_rest': 5.335757216853658, 'queen_rest': 0.5582884313170895, 'unused_tiles_rest': 0.138860394346906}. Best is trial 3 with value: 0.5.
[I 2021-06-10 18:04:05,382] Trial 18 finished with value: 0.435 and parameters: {'movement': 5.24814822817184, 'queen': 8.493435885602398, 'unused_tiles': 5.3504869710213745, 'movement_rest': 7.519225482592735, 'queen_rest': 0.051345605725487076, 'unused_tiles_rest': 7.360446562738851}. Best is trial 3 with value: 0.5.
[I 2021-06-10 18:06:43,710] Trial 19 finished with value: 0.435 and parameters: {'movement': 7.8250436987566925, 'queen': 0.3266257986906671, 'unused_tiles': 3.68198857904561, 'movement_rest': 2.4956512218029294, 'queen_rest': 6.588623417571078, 'unused_tiles_rest': 2.998334636179415}. Best is trial 3 with value: 0.5.
[I 2021-06-10 18:09:19,864] Trial 20 finished with value: 0.455 and parameters: {'movement': 6.07042766417373, 'queen': 7.1383396567307305, 'unused_tiles': 1.7091124655811507, 'movement_rest': 4.53192979919582, 'queen_rest': 2.2380902386693315, 'unused_tiles_rest': 4.852096228371138}. Best is trial 3 with value: 0.5.
[I 2021-06-10 18:11:59,567] Trial 21 finished with value: 0.47 and parameters: {'movement': 9.44541478638071, 'queen': 2.358643595781385, 'unused_tiles': 5.156293557440251, 'movement_rest': 5.188528260156831, 'queen_rest': 0.1107584844641192, 'unused_tiles_rest': 1.442702440719624}. Best is trial 3 with value: 0.5.
[I 2021-06-10 18:14:28,515] Trial 22 finished with value: 0.465 and parameters: {'movement': 3.026693214143377, 'queen': 3.546800724062553, 'unused_tiles': 5.66808129098576, 'movement_rest': 6.248441081280473, 'queen_rest': 1.9943604573238183, 'unused_tiles_rest': 1.710578934689553}. Best is trial 3 with value: 0.5.
[I 2021-06-10 18:16:59,564] Trial 23 finished with value: 0.475 and parameters: {'movement': 8.167007244440017, 'queen': 1.9728252081345983, 'unused_tiles': 6.14078537725578, 'movement_rest': 3.155484116335535, 'queen_rest': 3.9519901401826263, 'unused_tiles_rest': 3.1015222773375792}. Best is trial 3 with value: 0.5.
[I 2021-06-10 18:19:37,089] Trial 24 finished with value: 0.435 and parameters: {'movement': 6.844367113745687, 'queen': 1.1191567315050257, 'unused_tiles': 4.826868944341851, 'movement_rest': 5.7882182496576, 'queen_rest': 0.956634194413157, 'unused_tiles_rest': 0.8889124102809234}. Best is trial 3 with value: 0.5.
[I 2021-06-10 18:22:05,930] Trial 25 finished with value: 0.425 and parameters: {'movement': 9.977957097150925, 'queen': 3.0626776351906866, 'unused_tiles': 3.6119998179300663, 'movement_rest': 4.337691842380293, 'queen_rest': 0.017848296783085082, 'unused_tiles_rest': 0.02067971132768953}. Best is trial 3 with value: 0.5.
[I 2021-06-10 18:24:47,512] Trial 26 finished with value: 0.48 and parameters: {'movement': 3.8932067163698267, 'queen': 4.556752209287517, 'unused_tiles': 7.1925240842470775, 'movement_rest': 6.942449799916132, 'queen_rest': 1.7450845166312101, 'unused_tiles_rest': 3.241360137905322}. Best is trial 3 with value: 0.5.
[I 2021-06-10 18:27:27,174] Trial 27 finished with value: 0.465 and parameters: {'movement': 5.71462078576558, 'queen': 1.8113931203658855, 'unused_tiles': 4.559648154721298, 'movement_rest': 1.6783356912012888, 'queen_rest': 2.9386837928151572, 'unused_tiles_rest': 1.9182270395742693}. Best is trial 3 with value: 0.5.
[I 2021-06-10 18:30:05,917] Trial 28 finished with value: 0.515 and parameters: {'movement': 1.746088408894722, 'queen': 0.821447087541916, 'unused_tiles': 6.224338264884246, 'movement_rest': 4.955373114938358, 'queen_rest': 4.694355079936638, 'unused_tiles_rest': 4.45284268359465}. Best is trial 28 with value: 0.515.
[I 2021-06-10 18:32:25,208] Trial 29 finished with value: 0.36 and parameters: {'movement': 0.1954347954565261, 'queen': 0.6918805753755808, 'unused_tiles': 6.217126625890602, 'movement_rest': 8.238752592206964, 'queen_rest': 4.7129991169176035, 'unused_tiles_rest': 4.7104455493589725}. Best is trial 28 with value: 0.515.
[I 2021-06-10 18:35:02,360] Trial 30 finished with value: 0.505 and parameters: {'movement': 1.2095109112486007, 'queen': 0.040376067555271344, 'unused_tiles': 8.623878269174254, 'movement_rest': 4.905144100883239, 'queen_rest': 6.714104966955054, 'unused_tiles_rest': 6.984973072179815}. Best is trial 28 with value: 0.515.
[I 2021-06-10 18:37:47,452] Trial 31 finished with value: 0.495 and parameters: {'movement': 1.2869495807734015, 'queen': 0.08599231179318068, 'unused_tiles': 8.27111085183998, 'movement_rest': 4.977913491920262, 'queen_rest': 7.726442151571017, 'unused_tiles_rest': 6.566489988194829}. Best is trial 28 with value: 0.515.
[I 2021-06-10 18:40:34,603] Trial 32 finished with value: 0.47 and parameters: {'movement': 1.1262480100041588, 'queen': 0.10621200128127506, 'unused_tiles': 8.853048113075312, 'movement_rest': 4.749337951246501, 'queen_rest': 7.3467930456014905, 'unused_tiles_rest': 6.756190462771359}. Best is trial 28 with value: 0.515.
[I 2021-06-10 18:43:15,089] Trial 33 finished with value: 0.44 and parameters: {'movement': 1.8121283113872986, 'queen': 0.908231809546595, 'unused_tiles': 8.457803005620688, 'movement_rest': 4.865294500878839, 'queen_rest': 6.425333465997573, 'unused_tiles_rest': 8.577650649693597}. Best is trial 28 with value: 0.515.
[I 2021-06-10 18:45:48,428] Trial 34 finished with value: 0.47 and parameters: {'movement': 0.4981832878209729, 'queen': 0.21214852099467374, 'unused_tiles': 7.53894441831949, 'movement_rest': 6.57670637843281, 'queen_rest': 7.71652790825773, 'unused_tiles_rest': 7.785488717332221}. Best is trial 28 with value: 0.515.
[I 2021-06-10 18:48:24,347] Trial 35 finished with value: 0.475 and parameters: {'movement': 2.0550847838073194, 'queen': 1.341164860968852, 'unused_tiles': 8.031765159896393, 'movement_rest': 5.7085975387124845, 'queen_rest': 8.845571213093574, 'unused_tiles_rest': 5.9904194741945185}. Best is trial 28 with value: 0.515.
[I 2021-06-10 18:51:15,223] Trial 36 finished with value: 0.5 and parameters: {'movement': 1.3577648423420943, 'queen': 0.054413167595184914, 'unused_tiles': 8.95455271096435, 'movement_rest': 3.444092785106152, 'queen_rest': 6.612026839721651, 'unused_tiles_rest': 5.923119915768835}. Best is trial 28 with value: 0.515.
[I 2021-06-10 18:53:59,863] Trial 37 finished with value: 0.455 and parameters: {'movement': 0.015000514658298547, 'queen': 0.7110060445685459, 'unused_tiles': 9.016393557572433, 'movement_rest': 3.216167124402085, 'queen_rest': 6.560053218699563, 'unused_tiles_rest': 5.393223407319082}. Best is trial 28 with value: 0.515.
[I 2021-06-10 18:56:38,382] Trial 38 finished with value: 0.48 and parameters: {'movement': 3.2279785211637284, 'queen': 0.5355460647913344, 'unused_tiles': 9.94186633223726, 'movement_rest': 3.9861058207252453, 'queen_rest': 6.020478157344765, 'unused_tiles_rest': 4.312352541457736}. Best is trial 28 with value: 0.515.
[I 2021-06-10 18:59:16,268] Trial 39 finished with value: 0.52 and parameters: {'movement': 0.7824846134701269, 'queen': 0.017782940869814176, 'unused_tiles': 7.564628791731269, 'movement_rest': 2.1069259074058007, 'queen_rest': 4.586015025801454, 'unused_tiles_rest': 8.165761896885874}. Best is trial 39 with value: 0.52.
[I 2021-06-10 19:01:42,466] Trial 40 finished with value: 0.39 and parameters: {'movement': 0.8438808373033334, 'queen': 1.4934256876318188, 'unused_tiles': 7.562018269821236, 'movement_rest': 0.5216665049933509, 'queen_rest': 4.835073977254847, 'unused_tiles_rest': 9.917473673168008}. Best is trial 39 with value: 0.52.
[I 2021-06-10 19:04:24,052] Trial 41 finished with value: 0.485 and parameters: {'movement': 2.2885888534328718, 'queen': 0.019385454017425318, 'unused_tiles': 9.314262419276217, 'movement_rest': 2.506044701420901, 'queen_rest': 5.391626333214217, 'unused_tiles_rest': 8.357291751662489}. Best is trial 39 with value: 0.52.
[I 2021-06-10 19:07:05,874] Trial 42 finished with value: 0.48 and parameters: {'movement': 1.4174910941362522, 'queen': 0.9189623086002227, 'unused_tiles': 6.730999089215298, 'movement_rest': 1.8642952971528135, 'queen_rest': 7.091187236803729, 'unused_tiles_rest': 5.9938251394106254}. Best is trial 39 with value: 0.52.
[I 2021-06-10 19:09:53,257] Trial 43 finished with value: 0.525 and parameters: {'movement': 3.381903891288832, 'queen': 0.560710268927153, 'unused_tiles': 9.9549930491373, 'movement_rest': 2.9219674841871845, 'queen_rest': 4.35465520630387, 'unused_tiles_rest': 8.616202778642172}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:12:37,664] Trial 44 finished with value: 0.485 and parameters: {'movement': 4.567550846124843, 'queen': 0.6243072232151655, 'unused_tiles': 9.88823751166196, 'movement_rest': 9.842593820972008, 'queen_rest': 4.35630374398173, 'unused_tiles_rest': 8.814732519027135}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:15:31,486] Trial 45 finished with value: 0.495 and parameters: {'movement': 3.6902401743754925, 'queen': 1.3315336324998541, 'unused_tiles': 7.980266828609558, 'movement_rest': 2.1302968592914837, 'queen_rest': 3.2355822482079684, 'unused_tiles_rest': 9.217610418827402}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:18:12,011] Trial 46 finished with value: 0.47 and parameters: {'movement': 3.5511100484410907, 'queen': 2.477411538159906, 'unused_tiles': 7.426984641057073, 'movement_rest': 0.9293677855159157, 'queen_rest': 5.75830227740811, 'unused_tiles_rest': 7.681375066370471}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:20:39,519] Trial 47 finished with value: 0.425 and parameters: {'movement': 4.340287365568614, 'queen': 9.901728299597217, 'unused_tiles': 6.722462003005875, 'movement_rest': 3.9953520194604915, 'queen_rest': 4.042466420849796, 'unused_tiles_rest': 3.542506219920525}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:23:22,257] Trial 48 finished with value: 0.515 and parameters: {'movement': 2.5681250276786693, 'queen': 1.0298595452797674, 'unused_tiles': 8.519668838517013, 'movement_rest': 2.8176140466282398, 'queen_rest': 5.329659540031047, 'unused_tiles_rest': 8.03134049466133}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:25:57,912] Trial 49 finished with value: 0.47 and parameters: {'movement': 2.248973886221002, 'queen': 1.6430817579079555, 'unused_tiles': 9.468228599436163, 'movement_rest': 1.1709756634589614, 'queen_rest': 5.160997782620569, 'unused_tiles_rest': 8.138560529956665}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:28:22,062] Trial 50 finished with value: 0.51 and parameters: {'movement': 2.6620557224125934, 'queen': 0.004129177377935998, 'unused_tiles': 8.638217381898942, 'movement_rest': 2.800339817815108, 'queen_rest': 4.4078467562025585, 'unused_tiles_rest': 7.352795953996646}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:31:04,888] Trial 51 finished with value: 0.45 and parameters: {'movement': 2.723887287454538, 'queen': 0.4661293844018919, 'unused_tiles': 8.635300643454228, 'movement_rest': 2.8268028432110244, 'queen_rest': 4.468431250149759, 'unused_tiles_rest': 7.190343776155076}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:33:42,212] Trial 52 finished with value: 0.495 and parameters: {'movement': 1.792388669161068, 'queen': 1.0159056489750615, 'unused_tiles': 7.872733433675673, 'movement_rest': 2.095204391303252, 'queen_rest': 5.896364016251656, 'unused_tiles_rest': 9.113182891431329}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:36:18,452] Trial 53 finished with value: 0.48 and parameters: {'movement': 0.8027927624998304, 'queen': 0.0738586553186907, 'unused_tiles': 8.300587466418197, 'movement_rest': 3.5894063522457507, 'queen_rest': 3.4018616683540492, 'unused_tiles_rest': 7.8078798744917375}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:39:00,921] Trial 54 finished with value: 0.5 and parameters: {'movement': 3.2310416388686516, 'queen': 0.4459601692142849, 'unused_tiles': 9.765427945235608, 'movement_rest': 2.7217866312783348, 'queen_rest': 5.149643705105592, 'unused_tiles_rest': 8.19623490449877}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:41:32,419] Trial 55 finished with value: 0.41 and parameters: {'movement': 2.486782863202242, 'queen': 5.7137685727793945, 'unused_tiles': 9.269289447510438, 'movement_rest': 2.8534090414693214, 'queen_rest': 4.313502143497072, 'unused_tiles_rest': 7.289021687259459}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:44:11,306] Trial 56 finished with value: 0.47 and parameters: {'movement': 1.9143823350335902, 'queen': 1.1313054000514509, 'unused_tiles': 7.060693237093448, 'movement_rest': 2.1644301031309205, 'queen_rest': 3.7080650665940915, 'unused_tiles_rest': 6.4454846521540485}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:46:13,111] Trial 57 finished with value: 0.29 and parameters: {'movement': 0.39868660650661747, 'queen': 1.944835765450332, 'unused_tiles': 8.805160476929759, 'movement_rest': 3.736091327035438, 'queen_rest': 4.791775866378663, 'unused_tiles_rest': 9.746605375076172}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:48:21,990] Trial 58 finished with value: 0.33 and parameters: {'movement': 0.9821539996389581, 'queen': 3.2272452755455685, 'unused_tiles': 8.381945755515972, 'movement_rest': 1.416011094583599, 'queen_rest': 5.472542666263006, 'unused_tiles_rest': 9.40138331482144}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:50:58,450] Trial 59 finished with value: 0.485 and parameters: {'movement': 2.9747730412142945, 'queen': 0.02053361354459714, 'unused_tiles': 7.713783329617271, 'movement_rest': 0.7943885099016943, 'queen_rest': 6.177455463829352, 'unused_tiles_rest': 8.822004548244385}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:53:42,381] Trial 60 finished with value: 0.46 and parameters: {'movement': 1.587920587979677, 'queen': 0.4014865886644645, 'unused_tiles': 6.456514007611134, 'movement_rest': 3.0800043864911286, 'queen_rest': 2.850283140866284, 'unused_tiles_rest': 5.502654788971851}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:56:26,344] Trial 61 finished with value: 0.525 and parameters: {'movement': 3.3771123863941503, 'queen': 0.8500595340282492, 'unused_tiles': 6.134932714723548, 'movement_rest': 4.488839723280354, 'queen_rest': 4.20733269134546, 'unused_tiles_rest': 4.38150883762698}. Best is trial 43 with value: 0.525.
[I 2021-06-10 19:59:13,580] Trial 62 finished with value: 0.445 and parameters: {'movement': 3.386648467587149, 'queen': 0.882442232636393, 'unused_tiles': 5.89224855520106, 'movement_rest': 4.207373942686108, 'queen_rest': 4.183638086962603, 'unused_tiles_rest': 4.136549794329751}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:01:47,787] Trial 63 finished with value: 0.465 and parameters: {'movement': 2.566640249704761, 'queen': 1.4537625205581548, 'unused_tiles': 5.3699062269079585, 'movement_rest': 5.2562942288009165, 'queen_rest': 4.538791232642286, 'unused_tiles_rest': 4.5619118579646285}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:04:24,707] Trial 64 finished with value: 0.49 and parameters: {'movement': 3.955032529717498, 'queen': 1.693751669266259, 'unused_tiles': 7.034975179021201, 'movement_rest': 4.603320627308285, 'queen_rest': 3.630724210699544, 'unused_tiles_rest': 5.151660552586333}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:07:05,745] Trial 65 finished with value: 0.5 and parameters: {'movement': 2.211838263923759, 'queen': 2.219230932452144, 'unused_tiles': 7.298098666321219, 'movement_rest': 0.047446166824277825, 'queen_rest': 4.935470426468154, 'unused_tiles_rest': 3.7218585675287965}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:09:50,658] Trial 66 finished with value: 0.485 and parameters: {'movement': 3.0339663462069946, 'queen': 0.7040426856471781, 'unused_tiles': 5.9976374833431, 'movement_rest': 2.296351785384348, 'queen_rest': 3.1940726323724498, 'unused_tiles_rest': 7.539685494313924}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:12:25,331] Trial 67 finished with value: 0.46 and parameters: {'movement': 4.934409742972662, 'queen': 1.1119822585126704, 'unused_tiles': 9.140300866187106, 'movement_rest': 1.732566433762753, 'queen_rest': 5.534877810594403, 'unused_tiles_rest': 7.0152598497778484}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:15:01,410] Trial 68 finished with value: 0.52 and parameters: {'movement': 2.797828495445617, 'queen': 0.34977462728133757, 'unused_tiles': 6.489618145298264, 'movement_rest': 5.97473528889364, 'queen_rest': 3.930756983679308, 'unused_tiles_rest': 7.979713437793618}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:17:35,859] Trial 69 finished with value: 0.48 and parameters: {'movement': 2.8669529873217976, 'queen': 2.489280911839805, 'unused_tiles': 0.003032329737770567, 'movement_rest': 3.4330399328068864, 'queen_rest': 3.925795586268692, 'unused_tiles_rest': 7.96973113915165}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:20:17,741] Trial 70 finished with value: 0.48 and parameters: {'movement': 4.234008257684986, 'queen': 0.3122656082184061, 'unused_tiles': 6.393508804140023, 'movement_rest': 6.670036173709303, 'queen_rest': 2.3796937774060956, 'unused_tiles_rest': 8.794335092774968}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:22:53,835] Trial 71 finished with value: 0.495 and parameters: {'movement': 1.636441118461876, 'queen': 0.3157543793697028, 'unused_tiles': 6.597087661752019, 'movement_rest': 5.763059051290827, 'queen_rest': 4.627795367308925, 'unused_tiles_rest': 8.445369659602045}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:25:36,903] Trial 72 finished with value: 0.525 and parameters: {'movement': 2.584271559320456, 'queen': 0.01055634185036413, 'unused_tiles': 5.504451534559211, 'movement_rest': 5.507674530616574, 'queen_rest': 5.133957961034453, 'unused_tiles_rest': 6.291471820421768}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:28:16,142] Trial 73 finished with value: 0.465 and parameters: {'movement': 2.4770212074616107, 'queen': 0.7626483662471362, 'unused_tiles': 5.432629239944326, 'movement_rest': 5.884923610004195, 'queen_rest': 5.154845715089765, 'unused_tiles_rest': 6.246440831275432}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:30:58,065] Trial 74 finished with value: 0.495 and parameters: {'movement': 3.3624181327738065, 'queen': 1.1765569451035587, 'unused_tiles': 5.036072172349508, 'movement_rest': 5.483843924278881, 'queen_rest': 3.9205483452144385, 'unused_tiles_rest': 7.550727664435485}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:33:45,972] Trial 75 finished with value: 0.495 and parameters: {'movement': 2.042848294193985, 'queen': 0.5540526535533412, 'unused_tiles': 5.83722775848734, 'movement_rest': 6.037779204174262, 'queen_rest': 4.996988364360008, 'unused_tiles_rest': 5.124480352640595}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:36:20,051] Trial 76 finished with value: 0.435 and parameters: {'movement': 3.870582247657033, 'queen': 7.343273844721155, 'unused_tiles': 4.758925540934607, 'movement_rest': 6.5213842032580285, 'queen_rest': 4.151034629755823, 'unused_tiles_rest': 7.949622484255345}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:39:06,010] Trial 77 finished with value: 0.5 and parameters: {'movement': 2.622316322379817, 'queen': 0.016453181992853227, 'unused_tiles': 6.142729782670443, 'movement_rest': 5.130661867118677, 'queen_rest': 4.564256013641724, 'unused_tiles_rest': 2.6497445807253186}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:41:42,664] Trial 78 finished with value: 0.475 and parameters: {'movement': 3.54242961330013, 'queen': 1.367368717248729, 'unused_tiles': 5.542919403975084, 'movement_rest': 3.061776934236533, 'queen_rest': 3.5447457908821978, 'unused_tiles_rest': 6.733810681368639}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:44:21,345] Trial 79 finished with value: 0.465 and parameters: {'movement': 3.088230097431105, 'queen': 0.9400519980524342, 'unused_tiles': 4.146211674007263, 'movement_rest': 4.410781762949417, 'queen_rest': 2.6851491264244185, 'unused_tiles_rest': 8.254037677660946}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:46:59,820] Trial 80 finished with value: 0.49 and parameters: {'movement': 2.3082105306922713, 'queen': 1.8495633452579732, 'unused_tiles': 6.781985204852875, 'movement_rest': 2.4611362418194207, 'queen_rest': 5.36641831375432, 'unused_tiles_rest': 8.591732156526511}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:49:42,618] Trial 81 finished with value: 0.495 and parameters: {'movement': 1.1946950765655122, 'queen': 0.258589715024971, 'unused_tiles': 8.132530164426983, 'movement_rest': 4.700252393041853, 'queen_rest': 8.251579752075026, 'unused_tiles_rest': 6.993504507728522}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:52:22,664] Trial 82 finished with value: 0.51 and parameters: {'movement': 2.8748422168504613, 'queen': 0.6334093472101732, 'unused_tiles': 8.621814067297844, 'movement_rest': 4.8996865554265465, 'queen_rest': 4.406493669781484, 'unused_tiles_rest': 7.311791685405954}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:55:01,082] Trial 83 finished with value: 0.5 and parameters: {'movement': 2.827179345877391, 'queen': 0.860698713386379, 'unused_tiles': 7.310747078833934, 'movement_rest': 6.245423429651717, 'queen_rest': 4.340259730546773, 'unused_tiles_rest': 7.505393131635013}. Best is trial 43 with value: 0.525.
[I 2021-06-10 20:57:43,443] Trial 84 finished with value: 0.46 and parameters: {'movement': 4.042071882185677, 'queen': 0.5860259876210404, 'unused_tiles': 7.681880899456, 'movement_rest': 5.534568237620943, 'queen_rest': 4.6607423651757385, 'unused_tiles_rest': 8.021108848597457}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:00:16,302] Trial 85 finished with value: 0.465 and parameters: {'movement': 2.0106063853870144, 'queen': 0.25397736236242185, 'unused_tiles': 8.558613683545385, 'movement_rest': 4.1641614653467425, 'queen_rest': 3.3244693531735128, 'unused_tiles_rest': 4.413350170921437}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:02:58,409] Trial 86 finished with value: 0.49 and parameters: {'movement': 3.201173819210437, 'queen': 1.2546664182523894, 'unused_tiles': 5.189920096725014, 'movement_rest': 3.7006757236468495, 'queen_rest': 5.713872856100686, 'unused_tiles_rest': 4.890582637760621}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:05:35,177] Trial 87 finished with value: 0.525 and parameters: {'movement': 3.6338883578398633, 'queen': 1.589612652080683, 'unused_tiles': 9.571026646689957, 'movement_rest': 4.950459315282745, 'queen_rest': 3.7918217983956692, 'unused_tiles_rest': 9.124494314296324}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:08:15,366] Trial 88 finished with value: 0.5 and parameters: {'movement': 3.6194447221139145, 'queen': 1.6184114113280499, 'unused_tiles': 9.452532184828423, 'movement_rest': 4.948103939330615, 'queen_rest': 3.890256029235433, 'unused_tiles_rest': 9.099525122618216}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:10:52,552] Trial 89 finished with value: 0.435 and parameters: {'movement': 4.603656298821025, 'queen': 4.194518307848934, 'unused_tiles': 6.2400832572618254, 'movement_rest': 5.317404135072541, 'queen_rest': 4.129902507978859, 'unused_tiles_rest': 8.714449311094324}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:13:45,025] Trial 90 finished with value: 0.495 and parameters: {'movement': 3.4430615048836066, 'queen': 2.099167970476876, 'unused_tiles': 6.9574592965247, 'movement_rest': 4.518320233339676, 'queen_rest': 5.266311254573458, 'unused_tiles_rest': 5.726989608657255}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:16:24,621] Trial 91 finished with value: 0.51 and parameters: {'movement': 2.705096268295379, 'queen': 0.6151911727470699, 'unused_tiles': 8.768170302173782, 'movement_rest': 5.951807852413678, 'queen_rest': 3.7215102446588317, 'unused_tiles_rest': 7.323829870220947}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:19:04,167] Trial 92 finished with value: 0.48 and parameters: {'movement': 3.7508183973140907, 'queen': 0.6928931954279227, 'unused_tiles': 9.063315388060953, 'movement_rest': 5.949179354112725, 'queen_rest': 3.7473074563954683, 'unused_tiles_rest': 9.379025374149858}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:21:52,736] Trial 93 finished with value: 0.45 and parameters: {'movement': 2.5247421583992193, 'queen': 0.036621408570771785, 'unused_tiles': 9.609154041104206, 'movement_rest': 6.36534406251812, 'queen_rest': 4.910420482486908, 'unused_tiles_rest': 8.417024679495906}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:24:31,915] Trial 94 finished with value: 0.495 and parameters: {'movement': 2.327278960348344, 'queen': 1.0884558756330165, 'unused_tiles': 8.854039613201744, 'movement_rest': 6.877051707153989, 'queen_rest': 3.434932396784725, 'unused_tiles_rest': 9.015287971867078}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:27:16,687] Trial 95 finished with value: 0.525 and parameters: {'movement': 2.8334094787097346, 'queen': 0.4677956513063056, 'unused_tiles': 5.717943503474475, 'movement_rest': 5.144785296670818, 'queen_rest': 4.186665057114916, 'unused_tiles_rest': 7.73081034024944}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:30:03,730] Trial 96 finished with value: 0.5 and parameters: {'movement': 2.1178484216022486, 'queen': 0.36647855145714936, 'unused_tiles': 5.843484108990633, 'movement_rest': 5.154950887293223, 'queen_rest': 3.1991854302413416, 'unused_tiles_rest': 7.91972630368133}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:32:39,526] Trial 97 finished with value: 0.47 and parameters: {'movement': 1.5321752541678988, 'queen': 0.9153910686831206, 'unused_tiles': 5.638453297828621, 'movement_rest': 5.5312268049890525, 'queen_rest': 4.754088116686854, 'unused_tiles_rest': 3.7691766086466743}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:35:28,815] Trial 98 finished with value: 0.48 and parameters: {'movement': 2.8672438922828833, 'queen': 1.4361161517326597, 'unused_tiles': 5.1926016180829055, 'movement_rest': 4.870886046525079, 'queen_rest': 4.225630289434089, 'unused_tiles_rest': 7.6998811890315775}. Best is trial 43 with value: 0.525.
[I 2021-06-10 21:38:12,102] Trial 99 finished with value: 0.495 and parameters: {'movement': 3.2282476543801604, 'queen': 0.4909421862337661, 'unused_tiles': 9.975362239655402, 'movement_rest': 2.7432999410779697, 'queen_rest': 5.0684164173007415, 'unused_tiles_rest': 9.694387895128761}. Best is trial 43 with value: 0.525.
"""
    trial = []
    value = []
    pm = []
    pq = []
    pu = []
    pmr = []
    for m in re.finditer("Trial ([\d]+) finished with value: (\d\.[\d]+) and parameters: \{'movement': (\d\.[\d]+), 'queen': (\d\.[\d]+), 'unused_tiles': (\d\.[\d]+), 'movement_rest': (\d\.[\d]+), 'queen_rest': (\d\.[\d]+), 'unused_tiles_rest': (\d\.[\d]+)\}", data):
        trial.append(m.group(1))
        value.append(m.group(2))
        pm.append(m.group(3))
        pq.append(m.group(4))
        pu.append(m.group(5))
        pmr.append(m.group(6))

    trial = [int(t) for t in trial]
    value = [float(v) for v in value]

    pm = [float(v) for v in pm]
    pq = [float(v) for v in pq]
    pu = [float(v) for v in pu]

    pm = np.convolve(pm, [.25, .5, .25], "same")
    pq = np.convolve(pq, [.25, .5, .25], "same")
    pu = np.convolve(pu, [.25, .5, .25], "same")
    pmr = np.convolve(pmr, [.25, .5, .25], "same")

    # fig, ax1 = plt.subplots()
    plt.plot(trial, value, c="black")
    plt.hlines(.5, -1, 101)
    plt.xlabel("Trial")
    plt.ylabel("Win factor")
    plt.ylim((0, 1))
    plt.show()

    # ax2 = ax1.twinx()
    plt.plot(trial, pm)
    plt.plot(trial, pq)
    plt.plot(trial, pu)
    plt.plot(trial, pmr)
    plt.xlabel("Trial")
    plt.ylabel("Parameter Value")
    plt.ylim((0, 10))
    plt.legend(["Movement", "Queen", "Unused Tiles", "Distance to Queen"])
    plt.show()


if __name__ == "__main__":

    # plot()
    main()
