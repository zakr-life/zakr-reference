import pandas as pd

validation = pd.read_csv("synthetic_validation.csv")

passed = validation[validation["pcs_result"] == "PASS"]
failed = validation[validation["pcs_result"] != "PASS"]

print("Total:", len(validation))
print("Passed:", len(passed))
print("Failed:", len(failed))
