local M = assert(require 'csv.core')

function M.mt:done()
  local t, err = self:finish()
  if t == nil then
    return nil, err
  end

  local header = t[1]
  local m = #header

  local r = { header = header }

  for i = 2, #t do
    local ti, e = t[i], {}

    for j = 1, m do
      local tij = ti[j]

      if #tij > 0 then
        e[header[j]] = tij
      end
    end

    r[i-1] = e
  end

  return r
end

M.newline = '\r\n'

local function dump(t, write, nl)
  local header = t.header
  if type(header) ~= 'table' then
    return nil, 'Headers not found'
  end
  local n = #header

  if not nl then
    nl = M.newline
  end

  if not write then
    write = io.write
  end

  for i = 1, n-1 do
    write(header[i], ',')
  end
  write(header[n], nl)

  for i = 1, #t do
    local ti = t[i]
    for j = 1, n-1 do
      write(ti[header[j]] or '', ',')
    end
    write(ti[header[n]] or '', nl)
  end
end

M.dump = dump

function M.tostring(t, nl)
  local s, i = {}, 0
  local function concatter(a, b)
    if a then
      i = i + 1
      s[i] = a
    end

    i = i + 1
    s[i] = b
  end

  local r, err = dump(t, concatter, nl)
  if not r then
    return r, err
  end

  return table.concat(r)
end

return M

-- vi: syntax=lua ts=2 sw=2 et:
