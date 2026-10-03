import json,sys
W=120
def inline(v):
    if isinstance(v,dict):
        if not v: return "{ }"
        return "{ "+", ".join(json.dumps(k)+": "+inline(x) for k,x in v.items())+" }"
    if isinstance(v,list):
        if not v: return "[  ]"
        return "[ "+", ".join(inline(x) for x in v)+" ]"
    return json.dumps(v,ensure_ascii=False)
def fmt(v,ind,prefix_len):
    s=inline(v)
    if ind>0 and ind+prefix_len+len(s)<=W and not (isinstance(v,dict) and any(isinstance(x,(dict,list)) and len(inline(x))>60 for x in v.values())):
        return s
    if isinstance(v,dict):
        pad=" "*(ind+2)
        items=[pad+json.dumps(k)+": "+fmt(x,ind+2,len(json.dumps(k))+2) for k,x in v.items()]
        return "{\n"+",\n".join(items)+"\n"+" "*ind+"}"
    if isinstance(v,list):
        pad=" "*(ind+2)
        return "[\n"+",\n".join(pad+fmt(x,ind+2,0) for x in v)+"\n"+" "*ind+"]"
    return s
if __name__ == '__main__':
  for p in sys.argv[1:]:
    d=json.load(open(p))
    open(p,'w').write(fmt(d,0,0)+"\n")
