
import random

def MakeRandom3SatProb(numVars,numClauses):
    result = []
    for i in range(numClauses):
        row = [random.randint(1,numVars)]        
        for j in range(1,3):
            next = random.randint(1,numVars)
            while (row.count(next)):
                next = random.randint(1,numVars)
            row.append(next)
        for j in range(3):
            if (random.randint(0,1)):
                row[j]=-row[j]
        result.append(row)
    return result

def SatSolutionFailsP(prob,sol):
    for clause in range(len(prob)):
        ok = 0
        for var in prob[clause]:
            if ( (var > 0 and sol[var-1]) or (var < 0 and not sol[-var-1]) ):
                ok = 1
        if (not ok):
            return clause
    return -1
